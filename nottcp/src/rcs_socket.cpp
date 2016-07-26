#include "ucp.h"
#include "rcs_socket.h"
#include "rcs_exception.h"

using namespace std;

int RCSSocket::g_rcs_sock_counter = 0;
map<int, RCSSocket> RCSSocket::g_rcs_sockets;

/**
 * assign_rcs_sockfd - void
 *  assigns a new unique id to the provided RCSSocket and adds it to the gloal map.
 *  this id is used as the file descriptor for the conceptual rcs socket associated
 *  with this RCSSocket data structure.
 */
void assign_rcs_sockfd(RCSSocket &sock) {
    int id = RCSSocket::g_rcs_sock_counter;
    RCSSocket::g_rcs_sock_counter++;

    sock.id = id;
    RCSSocket::g_rcs_sockets.insert(pair<int, RCSSocket>(id, sock));
}


/**
 * create_rcs_sock - RCSSocket
 *  creates a RCS socket bound to a newly-created UCP socket
 */
int create_rcs_sock() {
    RCSSocket rcs_sock(ucpSocket());

    assign_rcs_sockfd(rcs_sock);

    return rcs_sock.id;
}

/**
 * create_bound_rcs_sock - RCSSocket
 *  creates a RCS socket bound to an existing UCP socket with the given sockfd
 */
int create_bound_rcs_sock(int sockfd) {
    RCSSocket rcs_sock(sockfd);

    assign_rcs_sockfd(rcs_sock);

    return rcs_sock.id;
}

/**
 * close_rcs_sock - int
 *  closes an RCS socket and the underlying UCP socket
 *  returns 0 on success
 */
int close_rcs_sock(int sockfd) {
    int result = ucpClose(RCSSocket::g_rcs_sockets.find(sockfd)->second.ucp_sockfd);

    if (result == 0) {
        RCSSocket::g_rcs_sockets.erase(sockfd);
    }

    return result;
}

/**
 * get_rcs_sock - RCSSocket
 *  returns the RCSSocket data structure associated with the provided sockfd
 */
RCSSocket get_rcs_sock(int sockfd) {
    map<int, RCSSocket>::iterator sockiter = RCSSocket::g_rcs_sockets.find(sockfd);

    if (sockiter == RCSSocket::g_rcs_sockets.end()) {
        throw RCSException(UNDEFINED_SOCKFD);
    }

    return sockiter->second;
}
