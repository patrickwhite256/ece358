/**
 * @brief: ECE358 RCS API interface dummy implementation
 *
 */
#include <cstring>
#include <cassert>
#include <iostream>
#include <algorithm>

#include "errno.h"

#include "rcs.h"
#include "ucp.h"

#include "rcs_socket.h"
#include "rcs_exception.h"
#include "message.h"

using namespace std;

int rcsSocket() {
    return RCSSocket::create();
}

int rcsBind(int sockfd, struct sockaddr_in *addr) {
    RCSSocket *rcs_sock;

    try {
        rcs_sock = RCSSocket::get(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    return(ucpBind(rcs_sock->ucp_sockfd, addr));
}

int rcsGetSockName(int sockfd, struct sockaddr_in *addr) {
    RCSSocket *rcs_sock;

    try {
        rcs_sock = RCSSocket::get(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    return ucpGetSockName(rcs_sock->ucp_sockfd, addr);
}

int rcsListen(int sockfd) {
    RCSSocket *rcs_sock;

    try {
        rcs_sock = RCSSocket::get(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    rcs_sock->state = RCS_STATE_LISTENING;

    return 0;
}

int rcsAccept(int sockfd, struct sockaddr_in *addr) {
    RCSSocket *listen_sock;

    try {
        listen_sock = RCSSocket::get(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    if (listen_sock->state != RCS_STATE_LISTENING) {
        //set errno
        return -1;
    }

    while(true) {
        Message *syn_msg = listen_sock->recv(true);
        memcpy(addr, listen_sock->cxn_addr, sizeof(sockaddr_in));
        if (syn_msg->is_syn()) {
            delete syn_msg;
            break;
        }
        delete syn_msg;
    }

    RCSSocket *rcs_sock = listen_sock->create_bound();
    rcs_sock->recv_seq_n = 1;

    Message *syn_ack = new Message(new char[0], 0, FLAG_SYN | FLAG_ACK);
    rcs_sock->send_q.push_back(syn_ack);

    rcs_sock->flush_send_q(); // send SYN-ACK and wait for ACK

    return rcs_sock->id;
}

int rcsConnect(int sockfd, const struct sockaddr_in *addr) {
    RCSSocket *rcs_sock;

    try {
        rcs_sock = RCSSocket::get(sockfd);
    } catch (RCSException e) {
        //set errno
        return -1;
    }

    memcpy(rcs_sock->cxn_addr, addr, sizeof(sockaddr_in));

    // TODO: give up eventually
    Message *syn = new Message(NULL, 0, FLAG_SYN);
    rcs_sock->send_q.push_back(syn);
    rcs_sock->flush_send_q();
    rcs_sock->state = RCS_STATE_SYN_SENT;

    Message *syn_ack = rcs_sock->recv();
    if (syn_ack->is_syn()) {
        delete syn_ack;
    } else {
        assert(false);
    }

    return 0;
}

int rcsRecv(int sockfd, void *buf, int len) {
    RCSSocket *rcs_sock;

    try {
        rcs_sock = RCSSocket::get(sockfd);
    } catch (RCSException e) {
        // set errno
        return -1;
    }

    if (rcs_sock->data_buf_size && rcs_sock->data_buf_size <= len) {
        // buffered data is lis less than is asked for
        // return what's there
        uint16_t buf_size = rcs_sock->data_buf_size;
        memcpy(buf, rcs_sock->data_buf, buf_size);

        delete[] rcs_sock->data_buf;
        rcs_sock->data_buf = NULL;
        rcs_sock->data_buf_size = 0;

        return buf_size;
    } else if (rcs_sock->data_buf_size > len) {
        // there is more buffered data than what is asked for
        // return the amount asked for and keep the rest buffered
        memcpy(buf, rcs_sock->data_buf, len);

        rcs_sock->data_buf_size -= len;
        char *excess = new char[rcs_sock->data_buf_size];
        memcpy(excess, &rcs_sock->data_buf[len], rcs_sock->data_buf_size);

        delete rcs_sock->data_buf;
        rcs_sock->data_buf = excess;

        return len;
    }

    // no buffered data, get a new message
    Message *msg = rcs_sock->recv();
    memcpy(buf, msg->content, len);

    if (msg->get_content_size() <= len) {
        // return what we got if that's all there is
        return msg->get_content_size();
    }

    // buffer any excess message content
    rcs_sock->data_buf_size = msg->get_content_size() - len;
    rcs_sock->data_buf =  new char[rcs_sock->data_buf_size];
    memcpy(rcs_sock->data_buf, &(msg->content[len]), rcs_sock->data_buf_size);
    return len;
}

int rcsSend(int sockfd, void *buf, int len) {
    RCSSocket *rcs_sock;

    try {
        rcs_sock = RCSSocket::get(sockfd);
    } catch (RCSException e) {
        // set errno
        return -1;
    }

    int max_content_size = MAX_UCP_PACKET_SIZE - HEADER_SIZE;

    // The return value will be wrong if this isn't true
    assert(rcs_sock->send_q.empty());

    for (int i = 0; i < len; i += max_content_size) {
        uint16_t msg_size = min(max_content_size, len - i * max_content_size);
        Message *msg = new Message(&((char *)buf)[i], msg_size, 0);
        rcs_sock->send_q.push_back(msg);
    }

    return rcs_sock->flush_send_q();
}

int rcsClose(int sockfd) {
    RCSSocket *rcs_sock;
    try {
        rcs_sock = RCSSocket::get(sockfd);
        rcs_sock->close();
    } catch (RCSException e) {
        return -1;
    }
}
