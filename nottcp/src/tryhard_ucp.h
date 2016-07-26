#ifndef TRYHARD_UCP
#define TRYHARD_UCP

#include <netinet/in.h>

int try_ucp_sendto(int sockfd, const void *buf, int size,
        const struct sockaddr_in *addr, int tries = 1, int timeout = 0);

int try_ucp_recvfrom(int sockfd, void *buf, int size, struct sockaddr_in *addr, int timeout);

#endif
