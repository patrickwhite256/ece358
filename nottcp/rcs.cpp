/**
 * @brief: ECE358 RCS API interface dummy implementation
 *
 */
#include "errno.h"

#include "rcs.h"
#include "ucp.h"

#include "rcs_socket.h"
#include "rcs_exception.h"

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

    // receive a connection initiation message
    // create a socket bound to the connected sockaddr
    // fill sockaddr inormation into addr and then return the new sockfd
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
