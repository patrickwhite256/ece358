#ifndef RCS_SOCKET
#define RCS_SOCKET

#include "ucp.h"
#include "message.h"

#include <queue>
#include <map>
#include <netinet/in.h>

#define RCS_STATE_NEW         0
#define RCS_STATE_LISTENING   1
#define RCS_STATE_SYN_SENT    2
#define RCS_STATE_SYN_RECV    3
#define RCS_STATE_ESTABLISHED 4

#define MAX_UCP_PACKET_SIZE 1000

struct RCSSocket {
    static int g_rcs_sock_counter;
    static std::map<int, RCSSocket *> g_rcs_sockets;

    int id;
    int ucp_sockfd;
    int remote_port;
    uint8_t state;
    sockaddr_in *cxn_addr;
    std::queue<Message *> messages;

    RCSSocket() : state(RCS_STATE_NEW) {
        cxn_addr = new sockaddr_in;
    }
    RCSSocket(int sockfd) : ucp_sockfd(sockfd), state(RCS_STATE_NEW) {
        cxn_addr = new sockaddr_in;
    }
    ~RCSSocket() { delete cxn_addr; }

    void send(Message &msg);
    Message *recv(void);

    void assign_sockfd();
    RCSSocket *create_bound();
    int close();

    static int create();
    static RCSSocket *get(int sockfd);
};

// TODO: destroy all sockets

struct RCSSocketException {
    int err_code;

    RCSSocketException(int err_code) : err_code(err_code) {}
};

#endif
