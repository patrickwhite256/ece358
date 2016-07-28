#include "ucp.h"
#include "rcs_socket.h"
#include "rcs_exception.h"

#include <cstring>
#include <cassert>
#include <iostream>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

using namespace std;

int RCSSocket::g_rcs_sock_counter = 0;
map<int, RCSSocket *> RCSSocket::g_rcs_sockets;


/**
 * assign_sockfd - void
 *  assigns a new unique id to the RCSSocket and adds it to the gloal map.
 *  this id is used as the file descriptor for the conceptual rcs socket associated
 *  with this RCSSocket data structure.
 */
void RCSSocket::assign_sockfd() {
    int id = RCSSocket::g_rcs_sock_counter;
    RCSSocket::g_rcs_sock_counter++;

    this->id = id;
    g_rcs_sockets.insert(pair<int, RCSSocket *>(id, this));
}


/**
 * create - int
 *  creates a RCS socket bound to a newly-created UCP socket
 *  return the rcs sockfd of the new socket.
 */
int RCSSocket::create() {
    RCSSocket *rcs_sock = new RCSSocket(ucpSocket());

    rcs_sock->assign_sockfd();

    return rcs_sock->id;
}

/**
 * create_bound - RCSSocket *
 *  creates a RCS socket bound to the same UCP socket.
 *  return the new socket
 */
RCSSocket *RCSSocket::create_bound() {
    RCSSocket *rcs_sock = new RCSSocket(ucp_sockfd);

    memcpy(rcs_sock->cxn_addr, cxn_addr, sizeof(struct sockaddr_in));
    rcs_sock->assign_sockfd();
    rcs_sock->remote_port = remote_port;

    return rcs_sock;
}

/**
 * close_rcs_sock - int
 *  closes an RCS socket and the underlying UCP socket
 *  returns 0 on success
 */
int RCSSocket::close() {
    // TODO: don't close ucp sockfd unless this is the listener
    int result = ucpClose(ucp_sockfd);

    if (result == 0) {
        RCSSocket::g_rcs_sockets.erase(this->id);
    }

    return result;
}

/**
 * get - RCSSocket *
 *  returns the RCSSocket data structure associated with the provided sockfd
 */
RCSSocket *RCSSocket::get(int sockfd) {
    auto sockiter = RCSSocket::g_rcs_sockets.find(sockfd);

    if (sockiter == RCSSocket::g_rcs_sockets.end()) {
        throw RCSException(RCS_ERROR_UNDEFINED_SOCKFD);
    }

    return sockiter->second;
}

void RCSSocket::send_ack() {
    Message *ack = new Message(NULL, 0, FLAG_ACK);
    ack->s_port = id;
    ack->d_port = remote_port;
    ack->set_akn(recv_seq_n);
    int b_sent = 0;
    uint8_t *ack_buf = ack->serialize();
    cout << "sending ack.     size:        " << +ack->size << endl;
    cout << "                 source port: " << +ack->s_port << endl;
    cout << "                 dest port:   " << +ack->d_port << endl;
    cout << "                 sequence #:  " << +(ack->flags & FLAG_SQN) << endl;
    while(b_sent != ack->size) {
        b_sent = ucpSendTo(ucp_sockfd, ack_buf, ack->size, cxn_addr);
    }

    if(last_ack != NULL) delete last_ack;
    last_ack = ack;
}

void RCSSocket::resend_ack() {
    uint8_t *ack_buf = last_ack->serialize();
    int b_sent = 0;
    while(b_sent != last_ack->size) {
        b_sent = ucpSendTo(ucp_sockfd, ack_buf, last_ack->size, cxn_addr);
    }
}

int RCSSocket::flush_send_q() {
    int sent = 0;

    while(!send_q.empty()) {
        Message *msg = send_q.front();

        msg->s_port = id;
        msg->d_port = remote_port;
        msg->set_sqn(send_seq_n);

        cout << "sending message. size:        " << +msg->size << endl;
        cout << "                 source port: " << +msg->s_port << endl;
        cout << "                 dest port:   " << +msg->d_port << endl;
        cout << "                 sequence #:  " << +(msg->flags & FLAG_SQN) << endl;

        const uint8_t *msg_buf = msg->serialize();
        int b_sent = ucpSendTo(ucp_sockfd, msg_buf, msg->size, cxn_addr);
        if(b_sent != msg->size) {
            // incomplete packet sent; retry
            cout << "did not send complete segment; resending" << endl;
            continue;
        }

        try {
            recv_ack();
        } catch(RCSException &ex) {
            if(ex.err_code == RCS_ERROR_TIMEOUT) {
                cout << "timeout on ack; resending." << endl;
                continue;
            } else if(ex.err_code == RCS_ERROR_CORRUPT) {
                cout << "corrupt ack; resending." << endl;
                continue;
            }
            assert(false);
        }

        sent += msg->get_content_size();

        send_seq_n = !send_seq_n;
        send_q.pop_front();
        delete msg;
    }

    return sent;
}

Message *RCSSocket::get_msg(uint16_t timeout) {
    uint16_t timeout_left = timeout;
    while(true) {
        // clear out ucp socket, then get something from the mq
        while(true) {
            unsigned char msg_buf[MAX_UCP_PACKET_SIZE];
            ucpSetSockRecvTimeout(ucp_sockfd, UCP_TIMEOUT_STEP_MS);
            int b_recv = ucpRecvFrom(ucp_sockfd, msg_buf, MAX_UCP_PACKET_SIZE, cxn_addr);
            if(b_recv < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)){
                // no more data
                break;
            }

            Message *msg = Message::deserialize(msg_buf, b_recv);

            cout << "received message. size:        " << +msg->size << endl;
            cout << "                  source port: " << +msg->s_port << endl;
            cout << "                  dest port:   " << +msg->d_port << endl;
            cout << "                  sequence #:  " << +(msg->flags & FLAG_SQN) << endl;

            try {
                msg->validate();
            } catch (RCSException &ex) {
                assert(ex.err_code == RCS_ERROR_CORRUPT);
                delete msg;
                throw;
            }

            // is this data for someone else
            if(state != RCS_STATE_LISTENING && msg->d_port != this->id){
                //TODO: maybe invalid by now
                RCSSocket *recipient = RCSSocket::get(msg->d_port);
                recipient->messages.push_back(msg);
                continue; // get new message
            }

            messages.push_back(msg);
        }

        if(!messages.empty()) {
            Message *msg = messages.front();
            messages.pop_front();
            return msg;
        }
        if(timeout > 0) {
            if(timeout_left < UCP_TIMEOUT_STEP_MS) throw RCSException(RCS_ERROR_TIMEOUT);
            timeout_left -= UCP_TIMEOUT_STEP_MS;
        }
    }
}

void RCSSocket::recv_ack() {
    Message *msg;
    while(true) { // wait until we get an in order data segment
        msg = get_msg(RESEND_TIMEOUT_MS);
        if(msg->is_ack()) {
            // must be in order ack
            assert(msg->get_akn() == send_seq_n);
        } else if(msg->get_sqn() == recv_seq_n) { // in order data packet
            messages.push_back(msg); //requeue
            continue;
        } else { // out of order data packet
            resend_ack();
            delete msg;
            continue; // get new packet
        }
        break; // in order ack
    }
    if(msg->is_syn()) {
        // requeue SYNACK as SYN
        msg->flags ^= FLAG_ACK;
        messages.push_back(msg);
    } else {
        delete msg;
    }
    cout << "received ack" << endl;
}

Message *RCSSocket::recv(bool no_ack) {
    Message *msg;
    while(true) { // wait until we get an in order data segment
        try {
            msg = get_msg();
        } catch (RCSException &ex) {
            assert(ex.err_code == RCS_ERROR_CORRUPT);
            cout << "received corrupt packet, discarding" << endl;
            continue;
        }
        if(msg->is_ack()) {
            // out of order ack. ignore, get new.
            assert(msg->get_akn() != send_seq_n);
            delete msg;
            continue; // get new packet
        } else if (msg->get_sqn() != recv_seq_n) {
            // out of order data
            resend_ack(); // resend ack
            delete msg;
            continue; // get new packet
        }
        break; // in order data
    }

    remote_port = msg->s_port;
    if(!no_ack) {
        send_ack();
    }
    recv_seq_n = !recv_seq_n;

    return msg;
}
