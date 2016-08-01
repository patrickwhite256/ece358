#ifndef RCS_SOCKET
#define RCS_SOCKET

#include "ucp.h"
#include "message.h"

#include <deque>
#include <map>
#include <netinet/in.h>
#include <mutex>

#define RCS_STATE_NEW         0
#define RCS_STATE_LISTENING   1
#define RCS_STATE_SYN_SENT    2
#define RCS_STATE_SYN_RECV    3
#define RCS_STATE_ESTABLISHED 4
#define RCS_STATE_FIN_WAIT    5
#define RCS_STATE_CLOSE_WAIT  6
#define RCS_STATE_TIME_WAIT   7
#define RCS_STATE_CLOSED      8

#define UCP_TIMEOUT_STEP_MS 10
#define MAX_UCP_PACKET_SIZE 1000

#define INITIAL_TIMEOUT       100
#define RTT_EST_UPDATE_FACTOR 0.125

struct RCSSocket {
    static int g_rcs_sock_counter;
    static std::map<int, RCSSocket *> g_rcs_sockets;
    static std::map<int, std::mutex *>  g_ucp_sock_mutexes;

    int id;
    int ucp_sockfd;
    int remote_port;
    uint8_t state;
    sockaddr_in *cxn_addr;
    std::deque<Message *> messages;
    std::deque<Message *> send_q;
    std::mutex messages_mutex;
    char *data_buf;
    uint16_t data_buf_size;
    Message *last_ack = NULL;

    uint8_t send_seq_n = 0;
    uint8_t recv_seq_n = 0;
    uint32_t est_rtt = INITIAL_TIMEOUT;

    RCSSocket() : state(RCS_STATE_NEW), cxn_addr(new sockaddr_in), data_buf_size(0) {}
    RCSSocket(int sockfd) : ucp_sockfd(sockfd), state(RCS_STATE_NEW), cxn_addr(new sockaddr_in), data_buf_size(0) {}
    ~RCSSocket() { delete cxn_addr; }

    int flush_send_q();
    Message *recv(bool no_ack = false);
    RCSSocket *create_bound();
    uint32_t get_timeout();

    void update_timeout(uint32_t rtt);
    void send_ack();
    void recv_ack();
    void resend_ack();
    void assign_sockfd();
    Message *get_msg(uint32_t timeout = 0);

    Message *safe_message_front();
    void safe_message_pop();
    void safe_message_push(Message *msg);
    bool safe_messages_empty();

    std::mutex *get_ucp_mutex();
    int safe_ucp_send(const void *buf, int size);
    int safe_ucp_recv(void *buf, int size);

    void timed_ack_wait();
    void fin_wait();
    int close();

    static int create();
    static RCSSocket *get(int sockfd);
};

// TODO: destroy all sockets

#endif
