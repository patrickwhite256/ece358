#ifndef RCS_SOCKET
#define RCS_SOCKET

#include "ucp.h"
#include "message.h"

#include <map>
#include <netinet/in.h>

#define RCS_STATE_NEW         0
#define RCS_STATE_LISTENING   1
#define RCS_STATE_SYN_SENT    2
#define RCS_STATE_SYN_RECV    3
#define RCS_STATE_ESTABLISHED 4

struct RCSSocket {
    static int g_rcs_sock_counter;
    static std::map<int, RCSSocket> g_rcs_sockets;

    int id;
    int ucp_sockfd;
    uint8_t state;
    sockaddr_in *cxn_addr;

    RCSSocket() : state(RCS_STATE_NEW), cxn_addr(NULL) {}
    RCSSocket(int sockfd) : ucp_sockfd(sockfd), state(RCS_STATE_NEW), cxn_addr(NULL) {}
    ~RCSSocket() { if (cxn_addr) delete cxn_addr; }

    void send(Message &msg);
    Message recv(void);
};

struct RCSSocketException {
    int err_code;

    RCSSocketException(int err_code) : err_code(err_code) {}
};

int create_rcs_sock();
int close_rcs_sock(int sockfd);
RCSSocket get_rcs_sock(int sockfd);

#endif
