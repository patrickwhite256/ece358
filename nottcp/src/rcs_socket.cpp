#include "ucp.h"
#include "rcs_socket.h"
#include "rcs_exception.h"

using namespace std;

int RCSSocket::g_rcs_sock_counter = 0;
map<int, RCSSocket> RCSSocket::g_rcs_sockets;

int create_rcs_sock() {
    RCSSocket rcs_sock(ucpSocket());

    int id = RCSSocket::g_rcs_sock_counter;
    RCSSocket::g_rcs_sock_counter++;

    rcs_sock.id = id;
    RCSSocket::g_rcs_sockets.insert(pair<int, RCSSocket>(id, rcs_sock));

    return id;
}

int close_rcs_sock(int sockfd) {
    int result = ucpClose(RCSSocket::g_rcs_sockets.find(sockfd)->second.ucp_sockfd);

    if (result == 0) {
        RCSSocket::g_rcs_sockets.erase(sockfd);
    }

    return result;
}

RCSSocket get_rcs_sock(int sockfd) {
    map<int, RCSSocket>::iterator sockiter = RCSSocket::g_rcs_sockets.find(sockfd);

    if (sockiter == RCSSocket::g_rcs_sockets.end()) {
        throw RCSException(UNDEFINED_SOCKFD);
    }

    return sockiter->second;
}

void RCSSocket::send(Message &msg) {
    const uint8_t *msg_buf = msg.serialize();
    int b_sent = ucpSendTo(ucp_sockfd, msg_buf, msg.size, cxn_addr);
    if(b_sent != msg.size) {
        // we screwed up, try again
    }
    delete[] msg_buf;
}

Message RCSSocket::recv() {
    unsigned char header_buf[HEADER_SIZE];
    int b_recv = ucpRecvFrom(ucp_sockfd, header_buf, HEADER_SIZE, cxn_addr);
    if(b_recv != HEADER_SIZE) {
        // corrupt
    }
    // deserialize header
    // validate_header
    // get body
    // check seq#
    // validate_body
    // return message
}
