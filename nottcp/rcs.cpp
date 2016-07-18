/**
 * @brief: ECE358 RCS API interface dummy implementation
 *
 */
#include "rcs.h"
#include "ucp.h"

#include "rcs_socket.h"

int rcsSocket() {
    return create_rcs_sock();
}

int rcsBind(int sockfd, struct sockaddr_in *addr)
{
	return -1;
}

int rcsGetSockName(int sockfd, struct sockaddr_in *addr)
{
	return -1;
}

int rcsListen(int sockfd)
{
	return -1;
}

int rcsAccept(int sockfd, struct sockaddr_in *addr)
{
	return -1;
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
    close_rcs_sock(sockfd);
    return 0;
}
