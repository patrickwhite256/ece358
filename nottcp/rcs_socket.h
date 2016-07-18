#ifndef RCS_SOCKET
#define RCS_SOCKET

#include <map>

struct RCSSocket {
    int id;
    int ucp_sockfd;

    RCSSocket(int sockfd) : ucp_sockfd(sockfd) {}
};

int g_rcs_sock_counter = 0;
std::map<int, RCSSocket> g_rcs_sockets;

int create_rcs_sock();
int close_rcs_sock(int sockfd);
RCSSocket get_rcs_sock(int sockfd);

#endif
