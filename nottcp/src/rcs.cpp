/**
 * @brief: ECE358 RCS API interface dummy implementation
 *
 */
#include <cstring>
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
    cout << "WHAT" << endl;

    while(true) {
        Message *syn_msg = listen_sock->recv();
        cout << "recv" << endl;
        memcpy(addr, listen_sock->cxn_addr, sizeof(sockaddr_in));
        if (syn_msg->flags & FLAG_SYN) {
            delete syn_msg;
            break;
        }
        delete syn_msg;
    }

    RCSSocket *rcs_sock = listen_sock->create_bound();

    Message syn_ack(new char[0], 0, FLAG_SYN | FLAG_ACK);
    rcs_sock->send(syn_ack);

    while (true) {
        Message *syn_msg = rcs_sock->recv();
        if (syn_msg->flags & FLAG_ACK) {
            delete syn_msg;
            break;
        }
        delete syn_msg;
    }

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

    Message syn(NULL, 0, FLAG_SYN);
    memcpy(rcs_sock->cxn_addr, addr, sizeof(sockaddr_in));
    rcs_sock->send(syn);
    rcs_sock->state = RCS_STATE_SYN_SENT;
    while (true) {
        // give up eventually
        Message *syn_ack_msg = rcs_sock->recv();
        cout << "WHAT" << endl;
        if ((syn_ack_msg->flags & FLAG_ACK) && (syn_ack_msg->flags & FLAG_SYN)) {
            delete syn_ack_msg;
            break;
        }
        delete syn_ack_msg;
    }

    Message ack(NULL, 0, FLAG_ACK);
    rcs_sock->send(ack);
    rcs_sock->state = RCS_STATE_ESTABLISHED;

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
