#include "ucp.h"
#include "rcs_socket.h"

int RCSSocket::g_rcs_sock_counter = 0;

int create_rcs_sock() {
    RCSSocket rcs_sock(ucpSocket());

    int id = RCSSocket::g_rcs_sock_counter;
    RCSSocket::g_rcs_sock_counter++;

    rcs_sock.id = id;
    RCSSocket::g_rcs_sockets.insert(std::pair<int, RCSSocket>(id, rcs_sock));

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
    return RCSSocket::g_rcs_sockets.find(sockfd)->second;
}
