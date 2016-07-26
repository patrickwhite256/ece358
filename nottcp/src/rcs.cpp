/**
 * @brief: ECE358 RCS API interface dummy implementation
 *
 */
#include <cstring>

#include "errno.h"

#include "rcs.h"
#include "ucp.h"

#include "rcs_socket.h"
#include "rcs_exception.h"
#include "message.h"
#include "tryhard_ucp.h"

int rcsSocket() {
    return create_rcs_sock();
}

int rcsBind(int sockfd, struct sockaddr_in *addr) {
    RCSSocket rcs_sock;

    try {
        rcs_sock = get_rcs_sock(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    return(ucpBind(rcs_sock.ucp_sockfd, addr));
}

int rcsGetSockName(int sockfd, struct sockaddr_in *addr) {
    RCSSocket rcs_sock;

    try {
        rcs_sock = get_rcs_sock(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    return ucpGetSockName(rcs_sock.ucp_sockfd, addr);
}

int rcsListen(int sockfd) {
    RCSSocket rcs_sock;

    try {
        rcs_sock = get_rcs_sock(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    rcs_sock.state = RCS_STATE_LISTENING;

    return 0;
}

int rcsAccept(int sockfd, struct sockaddr_in *addr) {
    RCSSocket rcs_sock;

    try {
        rcs_sock = get_rcs_sock(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    if (rcs_sock.state != RCS_STATE_LISTENING) {
        //set errno
        return -1;
    }

    // TODO: create new rcs socket bound to same addr

    bool got_syn = false;

    while (!got_syn) {
        Message syn_msg = rcs_sock.recv();
        if (syn_msg.flags & FLAG_SYN) {
            got_syn = true;
        }

        // something to give up eventually
    }

    Message syn_ack(new char[0], 0, FLAG_SYN | FLAG_ACK, 0); //TODO: rcs port
    rcs_sock.send(syn_ack);

    bool got_ack = false;

    while (!got_ack) {
        Message syn_msg = rcs_sock.recv();

        if (syn_msg.flags & FLAG_ACK) {

            got_ack = true;
        }

        // something to give up eventually
    }

    int cxn_sockfd = create_bound_rcs_sock(rcs_sock.ucp_sockfd);
    RCSSocket cxn = get_rcs_sock(cxn_sockfd);

    struct sockaddr_in cxn_addr;
    if (!rcsGetSockName(cxn_sockfd, &cxn_addr)) {
        // error
    }

    cxn.cxn_addr = new sockaddr_in;
    memcpy(cxn.cxn_addr, &cxn_addr, sizeof(struct sockaddr_in));

    return cxn_sockfd;

}

int rcsConnect(int sockfd, const struct sockaddr_in *addr) {
    RCSSocket rcs_sock;

    try {
        rcs_sock = get_rcs_sock(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    Message syn(NULL, 0, FLAG_SYN, 0); //TODO: rcs port
    rcs_sock.send(syn);
    rcs_sock.state = RCS_STATE_SYN_SENT;

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
    try {
        return close_rcs_sock(sockfd);
    } catch (RCSException e) {
        return -1;
    }
}
