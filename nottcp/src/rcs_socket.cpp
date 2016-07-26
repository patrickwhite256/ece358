#include "ucp.h"
#include "rcs_socket.h"
#include "rcs_exception.h"

#include <cstring>
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

void RCSSocket::send(Message &msg) {
    msg.s_port = id;
    msg.d_port = remote_port;
    cout << "sending message. source port: " << +msg.s_port << endl;
    cout << "                 dest port:   " << +msg.d_port << endl;
    const uint8_t *msg_buf = msg.serialize();
    int b_sent = ucpSendTo(ucp_sockfd, msg_buf, msg.size, cxn_addr);
    if(b_sent != msg.size) {
        // we screwed up, try again
    }
    delete[] msg_buf;
}

Message *RCSSocket::recv() {
    if(!messages.empty()) {
        Message *msg = messages.front();
        messages.pop();
        return msg;
    }
    while(true) { // until we receive a message meant for this socket
        unsigned char msg_buf[MAX_UCP_PACKET_SIZE];
        int b_recv;
        if(state == RCS_STATE_LISTENING) {
            b_recv = ucpRecvFrom(ucp_sockfd, msg_buf, HEADER_SIZE, cxn_addr);
        } else {
            b_recv = ucpRecvFrom(ucp_sockfd, msg_buf, HEADER_SIZE, NULL);
        }
        Message *msg = deserialize(msg_buf);
        msg->validate();
        cout << "received message. source port: " << +msg->s_port << endl;
        cout << "                  dest port:   " << +msg->d_port << endl;
        if(state != RCS_STATE_LISTENING && msg->d_port != this->id){
            RCSSocket *recipient = RCSSocket::get(msg->d_port);
            //TODO: maybe invalid
            recipient->messages.push(msg);
            continue;
        }
        remote_port = msg->s_port;
        // check seq#
        return msg;
    }
}
