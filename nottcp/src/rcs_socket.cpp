#include "ucp.h"
#include "rcs_socket.h"
#include "rcs_exception.h"

using namespace std;

int RCSSocket::g_rcs_sock_counter = 0;
map<int, RCSSocket *> RCSSocket::g_rcs_sockets;

/**
 * assign_sockfd - void
 *  assigns a new unique id to the RCSSocket and adds it to the gloal map.
 *  this id is used as the file descriptor for the conceptual rcs socket associated
 *  with this RCSSocket data structure.
 */
void RCSSocket::assign_sockfd() {
    int id = RCSSocket::g_rcs_sock_counter;
    RCSSocket::g_rcs_sock_counter++;

    this->id = id;
    g_rcs_sockets.insert(pair<int, RCSSocket *>(id, this));
}


/**
 * create - int
 *  creates a RCS socket bound to a newly-created UCP socket
 *  return the rcs sockfd of the new socket.
 */
int RCSSocket::create() {
    RCSSocket *rcs_sock = new RCSSocket(ucpSocket());

    rcs_sock->assign_sockfd();

    return rcs_sock->id;
}

/**
 * create_bound - RCSSocket *
 *  creates a RCS socket bound to the same UCP socket.
 *  return the new socket
 */
RCSSocket *RCSSocket::create_bound() {
    RCSSocket *rcs_sock = new RCSSocket(ucp_sockfd);

    rcs_sock->assign_sockfd();

    return rcs_sock;
}

/**
 * close_rcs_sock - int
 *  closes an RCS socket and the underlying UCP socket
 *  returns 0 on success
 */
int RCSSocket::close() {
    // TODO: don't close ucp sockfd unless this is the listener
    int result = ucpClose(ucp_sockfd);

    if (result == 0) {
        RCSSocket::g_rcs_sockets.erase(this->id);
    }

    return result;
}

/**
 * get - RCSSocket *
 *  returns the RCSSocket data structure associated with the provided sockfd
 */
RCSSocket *RCSSocket::get(int sockfd) {
    auto sockiter = RCSSocket::g_rcs_sockets.find(sockfd);

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

Message *RCSSocket::recv() {
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
