#include "ucp.h"
#include "rcs_socket.h"
#include "rcs_exception.h"

#include <cstring>
#include <cassert>
#include <iostream>

#include <sys/timeb.h>

using namespace std;

int RCSSocket::g_rcs_sock_counter = 0;
map<int, RCSSocket *> RCSSocket::g_rcs_sockets;


void print_msg_data(Message *msg, bool send) {
    const char * action = "send";
    if(!send) action = "recv";
    if(msg->flags & FLAG_ACK)
        cout << action << "ing ack.     size:        " << +msg->size << endl;
    else
        cout << action << "ing data.    size:        " << +msg->size << endl;
    cout << "                 source port: " << +msg->s_port << endl;
    cout << "                 dest port:   " << +msg->d_port << endl;
    cout << "                 flags:       " << +msg->flags << endl;
    cout << "                 sequence #:  " << +(msg->flags & FLAG_SQN) << endl;
    cout << "                 ack #:       " << +(msg->flags & FLAG_AKN) << endl;
    cout << "                 checksum:    " << msg->checksum << endl;
}


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
    // count the number of RCS sockets that are bound to the same ucp socket as this one
    int sock_count = 0;
    for (std::map<int, RCSSocket*>::iterator it = g_rcs_sockets.begin(); it != g_rcs_sockets.end(); ++it) {
        if (it->second->ucp_sockfd == ucp_sockfd) {
            sock_count++;
        }
    }

    int result = 0;

    // close the UCP socket if this is the last RCS socket bound to it
    if (sock_count == 1) {
        result = ucpClose(ucp_sockfd);
    }

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
#ifdef DEBUG
    cout << "Sending ack." << endl;
#endif
    Message *ack = new Message(NULL, 0, FLAG_ACK);
    ack->s_port = id;
    ack->d_port = remote_port;
    ack->set_akn(recv_seq_n);

    if (state == RCS_STATE_CLOSE_WAIT || state == RCS_STATE_FIN_WAIT) {
        ack->flags |= FLAG_FIN;
    }

    int b_sent = 0;
    uint8_t *ack_buf = ack->serialize();
    while(b_sent != ack->size) {
#ifdef VERBOSE
        print_msg_data(ack, true);
#endif
        b_sent = ucpSendTo(ucp_sockfd, ack_buf, ack->size, cxn_addr);
    }

    if(last_ack != NULL) delete last_ack;
    last_ack = ack;
}

void RCSSocket::resend_ack() {
#ifdef DEBUG
    cout << "Resending ack." << endl;
#endif
    uint8_t *ack_buf = last_ack->serialize();
    int b_sent = 0;
    while(b_sent != last_ack->size) {
#ifdef VERBOSE
        print_msg_data(last_ack, true);
#endif
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
        const uint8_t *msg_buf = msg->serialize();

#ifdef VERBOSE
        print_msg_data(msg, true);
#endif
#ifdef DEBUG
        cout << "Sending data." << endl;
#endif

        int b_sent = ucpSendTo(ucp_sockfd, msg_buf, msg->size, cxn_addr);
        if(b_sent != msg->size) {
            // incomplete packet sent; retry
#ifdef DEBUG
            cout << "did not send complete segment; resending" << endl;
#endif
            continue;
        }

        timeb time_sent, time_recv;
        ftime(&time_sent);
        try {
            recv_ack();
        } catch(RCSException &ex) {
            if(ex.err_code == RCS_ERROR_TIMEOUT) {
#ifdef DEBUG
                cout << "timeout on ack; resending." << endl;
                update_timeout(get_timeout()); // increase timeout
#endif
                continue;
            } else if(ex.err_code == RCS_ERROR_CORRUPT) {
#ifdef DEBUG
                cout << "corrupt ack; resending." << endl;
#endif
                continue;
            } else if(ex.err_code == RCS_ERROR_RESEND) {
#ifdef DEBUG
                cout << "in order data while waiting for ack; resending" << endl;
#endif
                continue;
            }
            assert(false);
        }
        ftime(&time_recv);
        uint32_t rtt = (time_recv.time - time_sent.time) * 1000 +
                       (time_recv.millitm - time_sent.millitm);
        update_timeout(rtt);

        sent += msg->get_content_size();

        send_seq_n = !send_seq_n;
#ifdef DEBUG
        cout << "SEND SQN: " << +send_seq_n << endl;
#endif
        send_q.pop_front();
        delete msg;
    }

    return sent;
}

Message *RCSSocket::get_msg(uint32_t timeout) {
    uint32_t timeout_left = timeout;
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

            try {
                msg->validate();
            } catch (RCSException &ex) {
#ifdef DEBUG
                cout << "received corrupt data" << endl;
#endif
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
#ifdef VERBOSE
            print_msg_data(msg, false);
#endif
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
        msg = get_msg(get_timeout());
        if(msg->is_ack() && msg->get_akn() != send_seq_n) { //out of order ack
            delete msg;
            continue;
        } else if(!msg->is_ack() && msg->get_sqn() == recv_seq_n) { // in order data packet
            delete msg;
            throw RCSException(RCS_ERROR_RESEND); // resend data
        } else if(!msg->is_ack()){ // out of order data packet
#ifdef DEBUG
            cout << "received out of order data, resending ack" << endl;
#endif
            resend_ack();
            delete msg;
            continue; // get new packet
        }
        break; // in order ack
    }
#ifdef DEBUG
    cout << "Received ack" << endl;
#endif
}

Message *RCSSocket::recv(bool no_ack) {
    Message *msg;
    while(true) { // wait until we get an in order data segment
        try {
            msg = get_msg();
        } catch (RCSException &ex) {
            assert(ex.err_code == RCS_ERROR_CORRUPT);
#ifdef DEBUG
            cout << "received corrupt packet, discarding" << endl;
#endif
            continue;
        }
        if(msg->is_ack()) {
            // out of order ack. ignore, get new.
#ifdef DEBUG
            cout << "received out of order data, ignoring" << endl;
#endif
            assert(msg->get_akn() != send_seq_n);
            delete msg;
            continue; // get new packet
        } else if (msg->get_sqn() != recv_seq_n) {
#ifdef DEBUG
            cout << "received out of order data, resending ack" << endl;
#endif
            // out of order data
            resend_ack(); // resend ack
            delete msg;
            continue; // get new packet
        }
        break; // in order data
    }
#ifdef DEBUG
    cout << "Received data" << endl;
#endif

    remote_port = msg->s_port;

    if (msg->is_fin()) {
        state = RCS_STATE_CLOSE_WAIT;
    }

    if(!no_ack) {
        send_ack();
    }

    recv_seq_n = !recv_seq_n;
#ifdef DEBUG
    cout << "RECV SQN: " << +recv_seq_n << endl;
#endif

    return msg;
}

void RCSSocket::update_timeout(uint32_t rtt) {
    est_rtt = (1 - RTT_EST_UPDATE_FACTOR) * est_rtt + RTT_EST_UPDATE_FACTOR * rtt;
}

uint32_t RCSSocket::get_timeout() {
    return est_rtt * 2;
}

void RCSSocket::fin_wait() {
    // throw out messages until we get a FIN
    while (true) {
        Message *fin = recv();

        if (fin->is_fin()) {
            state = RCS_STATE_TIME_WAIT;
            delete fin;
            break;
        }

        delete fin;
    }

    // wait until we timeout before we assume our ack was recieved
    while (true) {
        try {
            Message *resent_fin = get_msg(2 * RESEND_TIMEOUT_MS);
            resend_ack();
            delete resent_fin;
        } catch (RCSException e) {
            break;
        }
    }
}
