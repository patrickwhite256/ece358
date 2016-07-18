#include "ucp.h"
#include "rcs_socket.h"

int create_rcs_sock() {
    RCSSocket rcs_sock(ucpSocket());

    int id = g_rcs_sock_counter;
    g_rcs_sock_counter++;

    rcs_sock.id = id;
    g_rcs_sockets.insert(std::pair<int, RCSSocket>(id, rcs_sock));

    return id;
}

void close_rcs_sock(int sockfd) {
    ucpClose(g_rcs_sockets.find(sockfd)->second.ucp_sockfd);
    g_rcs_sockets.erase(sockfd);
}
