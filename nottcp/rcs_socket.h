#ifndef RCS_SOCKET
#define RCS_SOCKET

#include <map>

struct RCSSocket {
    static int g_rcs_sock_counter;
    static std::map<int, RCSSocket> g_rcs_sockets;

    int id;
    int ucp_sockfd;
    bool is_listening;

    RCSSocket() : is_listening(false) {}
    RCSSocket(int sockfd) : ucp_sockfd(sockfd), is_listening(false) {}
};

struct RCSSocketException {
    int err_code;

    RCSSocketException(int err_code) : err_code(err_code) {}
};

int create_rcs_sock();
int close_rcs_sock(int sockfd);
RCSSocket get_rcs_sock(int sockfd);

#endif
