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

    rcs_sock.is_listening = true;

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

    if (!rcs_sock.is_listening) {
        //set errno
        return -1;
    }

    bool got_syn = false;

    while (!got_syn) {
        uint8_t *buf = new uint8_t[HEADER_SIZE];
        try_ucp_recvfrom(rcs_sock.ucp_sockfd, (void *)buf, HEADER_SIZE, addr, 0);

        Message syn_msg = deserialize(buf);

        if (syn_msg.validate()          &&
            (syn_msg.flags & FLAG_SYN)  &&
            !(syn_msg.flags & FLAG_SQN)) {

            got_syn = true;
        }

        // something to give up eventually
    }

    Message syn_ack(new char[0], 0, FLAG_SYN | FLAG_ACK, addr->sin_port);
    const uint8_t *syn_ack_buf = syn_ack.serialize();
    try_ucp_sendto(rcs_sock.ucp_sockfd, (const void *)syn_ack_buf, syn_ack.size, addr, 1, 0);
    delete[] syn_ack_buf;

    bool got_ack = false;

    while (!got_ack) {
        uint8_t *buf = new uint8_t[HEADER_SIZE];
        try_ucp_recvfrom(rcs_sock.ucp_sockfd, (void *)buf, HEADER_SIZE, addr, 0);

        Message syn_msg = deserialize(buf);

        if (syn_msg.validate()          &&
            (syn_msg.flags & FLAG_ACK)  &&
            (syn_msg.flags & FLAG_SQN)) {

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

int rcsConnect(int sockfd, const struct sockaddr_in *addr)
{
	return -1;
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
