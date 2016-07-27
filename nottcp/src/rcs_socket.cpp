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
        throw RCSException(UNDEFINED_SOCKFD);
    }

    return sockiter->second;
}

void RCSSocket::send_ack() {
    Message *ack = new Message(NULL, 0, FLAG_ACK);
    ack->s_port = id;
    ack->d_port = remote_port;
    ack->flags ^= recv_seq_n;
    int b_sent = 0;
    uint8_t *ack_buf = ack->serialize();
    while(b_sent != ack->size) {
        b_sent = ucpSendTo(ucp_sockfd, ack_buf, ack->size, cxn_addr);
    }

    last_ack = ack;
}

void RCSSocket::resend_ack() {
    uint8_t *ack_buf = last_ack->serialize();
    int b_sent = 0;
    while(b_sent != last_ack->size) {
        b_sent = ucpSendTo(ucp_sockfd, ack_buf, last_ack->size, cxn_addr);
    }
}

void RCSSocket::flush_send_q() {
    while(!send_q.empty()) {
        Message *msg = send_q.front();

        msg->s_port = id;
        msg->d_port = remote_port;
        msg->flags ^= send_seq_n;


        cout << "sending message. source port: " << +msg->s_port << endl;
        cout << "                 dest port:   " << +msg->d_port << endl;

        const uint8_t *msg_buf = msg->serialize();
        int b_sent = ucpSendTo(ucp_sockfd, msg_buf, msg->size, cxn_addr);
        if(b_sent != msg->size) {
            // incomplete packet sent; retry
            continue;
        }

        while(true) { // while haven't received ack
            // TODO: timeout
            Message *ack = recv(true);
            if(ack->flags & FLAG_ACK == 0) {
                if(ack->flags & FLAG_SQN == recv_seq_n) {
                    // received data in sequence while expecting ack;
                    messages.push_back(ack); //queue the data
                } else {
                    // received data out of sequence - our ack was lost
                    resend_ack(); // resend last ack
                    delete ack;
                }
                // in both cases, continue waiting for ack
                continue;
            }
            assert(ack->flags & FLAG_AKN == send_seq_n);
            break;
        }

        send_seq_n ^= FLAG_SQN;
        send_q.pop_front();
        delete msg;
    }
}

void RCSSocket::recv_ack() {

}

Message *RCSSocket::recv(bool no_ack) {
    Message *msg;
    while(true) { // wait until we get an in order data segment
        if(!messages.empty()) {
            msg = messages.front();
            messages.pop_front();
        } else {
            while(true) { // until we receive a message meant for this socket
                unsigned char msg_buf[MAX_UCP_PACKET_SIZE];
                int b_recv = ucpRecvFrom(ucp_sockfd, msg_buf, HEADER_SIZE, cxn_addr);

                msg = deserialize(msg_buf);

                cout << "received message. source port: " << +msg->s_port << endl;
                cout << "                  dest port:   " << +msg->d_port << endl;

                if(!msg->validate()) {
                    cout << "corrupt packet. ignoring" << endl;
                    // ignore
                    delete msg;
                    continue;
                }

                if(state != RCS_STATE_LISTENING && msg->d_port != this->id){
                    // this is data for someone else
                    RCSSocket *recipient = RCSSocket::get(msg->d_port);
                    //TODO: maybe invalid
                    recipient->messages.push_back(msg);
                    continue; // get new message
                }

                break;
            }
        }
        if(msg->flags & FLAG_ACK) {
            // out of order ack. ignore, get new.
            assert(msg->flags & FLAG_AKN != send_seq_n);
            delete msg;
            continue; // get new packet
        } else if (msg->flags & FLAG_SQN != recv_seq_n) {
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
    recv_seq_n ^= FLAG_ACK;

    return msg;
}
