#ifndef RCS_SOCKET
#define RCS_SOCKET

#include "ucp.h"
#include "message.h"

#include <deque>
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
    std::deque<Message *> messages;
    std::deque<Message *> send_q;
    char *data_buf;
    uint16_t data_buf_size;
    Message *last_ack = NULL;

    uint8_t send_seq_n = 0;
    uint8_t recv_seq_n = 0;

    RCSSocket() : state(RCS_STATE_NEW), data_buf_size(0) {
        cxn_addr = new sockaddr_in;
    }
    RCSSocket(int sockfd) : ucp_sockfd(sockfd), state(RCS_STATE_NEW), data_buf_size(0) {

        cxn_addr = new sockaddr_in;
    }
    ~RCSSocket() { delete cxn_addr; }

    void send_ack();
    void recv_ack();
    void resend_ack();
    void flush_send_q();
    Message *recv(bool no_ack = false);

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
