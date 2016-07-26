/**
 * @brief: ECE358 RCS API interface dummy implementation
 *
 */
#include <cstring>
#include <cassert>
#include <iostream>

#include "errno.h"

#include "rcs.h"
#include "ucp.h"

#include "rcs_socket.h"
#include "rcs_exception.h"
#include "message.h"

using namespace std;

int rcsSocket() {
    return RCSSocket::create();
}

int rcsBind(int sockfd, struct sockaddr_in *addr) {
    RCSSocket *rcs_sock;

    try {
        rcs_sock = RCSSocket::get(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    return(ucpBind(rcs_sock->ucp_sockfd, addr));
}

int rcsGetSockName(int sockfd, struct sockaddr_in *addr) {
    RCSSocket *rcs_sock;

    try {
        rcs_sock = RCSSocket::get(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    return ucpGetSockName(rcs_sock->ucp_sockfd, addr);
}

int rcsListen(int sockfd) {
    RCSSocket *rcs_sock;

    try {
        rcs_sock = RCSSocket::get(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    rcs_sock->state = RCS_STATE_LISTENING;

    return 0;
}

int rcsAccept(int sockfd, struct sockaddr_in *addr) {
    RCSSocket *listen_sock;

    try {
        listen_sock = RCSSocket::get(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    if (listen_sock->state != RCS_STATE_LISTENING) {
        //set errno
        return -1;
    }

    cout << "ucp_sockfd: " << listen_sock->ucp_sockfd << endl;

    while(true) {
        Message *syn_msg = listen_sock->recv(true);
        cout << "recv" << endl;
        memcpy(addr, listen_sock->cxn_addr, sizeof(sockaddr_in));
        if (syn_msg->flags & FLAG_SYN) {
            delete syn_msg;
            break;
        }
        delete syn_msg;
    }

    RCSSocket *rcs_sock = listen_sock->create_bound();
    rcs_sock->recv_seq_n = 1;

    Message *syn_ack = new Message(new char[0], 0, FLAG_SYN | FLAG_ACK);
    rcs_sock->send_q.push_back(syn_ack);

    rcs_sock->flush_send_q(); // send SYN-ACK and wait for ACK

    return rcs_sock->id;
}

int rcsConnect(int sockfd, const struct sockaddr_in *addr) {
    RCSSocket *rcs_sock;

    try {
        rcs_sock = RCSSocket::get(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    memcpy(rcs_sock->cxn_addr, addr, sizeof(sockaddr_in));

    // give up eventually
    Message *syn = new Message(NULL, 0, FLAG_SYN);
    rcs_sock->send_q.push_back(syn);
    rcs_sock->flush_send_q();
    rcs_sock->state = RCS_STATE_SYN_SENT;

    cout << "waiting for synack" << endl;
    cout << rcs_sock->messages.size() << endl;
    Message *syn_ack = rcs_sock->recv();
    if (syn_ack->flags & FLAG_SYN) {
        delete syn_ack;
    } else {
        cout << "fuck" << endl;
        assert(false);
    }

    return 0;
}

int rcsRecv(int sockfd, void *buf, int len)
{
	return -1;
}

int rcsSend(int sockfd, void *buf, int len)
{
	return -1;
}

int rcsClose(int sockfd) {
    RCSSocket *rcs_sock;
    try {
        rcs_sock = RCSSocket::get(sockfd);
        rcs_sock->close();
    } catch (RCSException e) {
        return -1;
    }
}
