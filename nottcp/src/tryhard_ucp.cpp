#include "tryhard_ucp.h"
#include "ucp.h"

int try_ucp_sendto(int sockfd, const void *buf, int size,
    const struct sockaddr_in *addr, int tries, int timeout) {

    return ucpSendTo(sockfd, buf, size, addr);
}

int try_ucp_recvfrom(int sockfd, void *buf, int size, struct sockaddr_in *addr, int timeout) {
    return ucpRecvFrom(sockfd, buf, size, addr);
}
