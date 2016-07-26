#ifndef RCS_SOCKET
#define RCS_SOCKET

#include <map>

struct RCSSocket {
    static int g_rcs_sock_counter;
    static std::map<int, RCSSocket> g_rcs_sockets;

    int id;
    int ucp_sockfd;
    bool is_listening;
    sockaddr_in *cxn_addr;

    RCSSocket() : is_listening(false), cxn_addr(NULL) {}
    RCSSocket(int sockfd) : ucp_sockfd(sockfd), is_listening(false), cxn_addr(NULL) {}
    ~RCSSocket() { delete cxn_addr; }
};

struct RCSSocketException {
    int err_code;

    RCSSocketException(int err_code) : err_code(err_code) {}
};

int create_rcs_sock();
int create_bound_rcs_sock(int sockfd);
int close_rcs_sock(int sockfd);
RCSSocket get_rcs_sock(int sockfd);

#endif
