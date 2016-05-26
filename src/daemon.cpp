#include <cstdio>
#include <cstring>
#include <algorithm>
#include <arpa/inet.h>
#include <iostream>
#include <netinet/tcp.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>

#include "daemon.h"
#include "peer.h"
#include "basic_exception.h"

const int INITIAL_BUFFER_LEN = 300;
const char *ALL_KEYS         = "allkeys";
const char *ADD_KEY          = "addakey";
const char *REQUEST_INFO     = "reqinfo";
const char *PEER_DATA        = "peerdta";

void die_on_error() {
    std::perror("error:");
    exit(-1);
}

Daemon::Daemon(int sockfd) {
    this->sockfd = sockfd;
    this->peer_id = 0; // assume we are the first node until told otherwise
    sockaddr_in dummy;
    this->peer_set = new Peer(dummy, 0, 0);
}

void Daemon::loop() {
    if(listen(sockfd, 0) < 0) {
        die_on_error();
    }

    while(true) {
        Daemon::Message *message = receive_message();
        //handle message
        delete message;
    }
}

Daemon::Message *Daemon::receive_message() {
    unsigned char initial_buffer[INITIAL_BUFFER_LEN];
    sockaddr_in client;
    socklen_t alen = sizeof(sockaddr_in);
    int connectedsock = accept(sockfd, (sockaddr *)&client, &alen);
    if(connectedsock < 0) {
        die_on_error();
    }
    ssize_t recv_len = recv(connectedsock, initial_buffer,
                            INITIAL_BUFFER_LEN, 0);
    if(recv_len < 0) {
        die_on_error();
    }

    char *cmd = new char[8];
    memcpy(cmd, initial_buffer, 7);
    cmd[7] = '\0';

    if(strcmp(cmd, ALL_KEYS) == 0) {
        char *dummy = new char[1];
        return new Daemon::Message(cmd, 0, dummy, client);
    } else {
        int msg_size = (initial_buffer[7] << 8) + initial_buffer[8];
        char *contents = new char[msg_size + 1];
        int msg_remaining = msg_size;
        int amt_from_first_packet = std::min(msg_remaining, INITIAL_BUFFER_LEN - 9);
        memcpy(contents, &initial_buffer[9], amt_from_first_packet);
        msg_remaining -= amt_from_first_packet;
        while(msg_remaining) {
            recv_len = recv(connectedsock, &contents[msg_size - msg_remaining],
                            msg_remaining, 0);
            msg_remaining -= recv_len;
        }
        contents[msg_size] = '\0';

        return new Daemon::Message(cmd, msg_size, contents, client);
    }
}

void Daemon::send_command(const char *cmd_id, char *cmd_body, int body_len, sockaddr_in *dest) {
    int sockfd = -1;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        die_on_error();
    }

    struct sockaddr_in client;
    bzero(&client, sizeof(struct sockaddr_in));
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_port = 0;
    if(bind(sockfd, (struct sockaddr *)&client, sizeof(struct sockaddr_in)) < 0) {
        die_on_error();
    }

    socklen_t alen = sizeof(struct sockaddr_in);
    if(getsockname(sockfd, (struct sockaddr *)&client, &alen) < 0) {
        die_on_error();
    }

    if(::connect(sockfd, (struct sockaddr *)dest, sizeof(struct sockaddr_in)) < 0) {
        throw Exception(BAD_ADDRESS);
    }

    size_t msglen = body_len + 9;
    char msg[msglen];
    memcpy(msg, cmd_id, 7);

    msg[7] = body_len >> 8;
    msg[8] = body_len - (msg[7] << 8);

    if (body_len > 0) {
        strcpy(&msg[9], cmd_body);
    }

    ssize_t sentlen;
    if((sentlen = send(sockfd, msg, msglen, 0)) < 0 ) {
        die_on_error();
    }

    if(shutdown(sockfd, SHUT_RDWR) < 0) {
        die_on_error();
    }
}

void Daemon::broadcast(const char *cmd_id, char *cmd_body, int body_len) {
    Peer *next = peer_set;

    do {
        if (next->id != this->peer_id) {
            send_command(cmd_id, cmd_body, body_len, &next->address);
        }
        next = next->next;
    } while (next != peer_set);
}

void Daemon::connect(const char *remote_ip, int remote_port) {
    in_addr addr;
    sockaddr_in remote;
    inet_aton(remote_ip, &addr);
    remote.sin_family = AF_INET;
    remote.sin_addr = addr;
    remote.sin_port = remote_port;
    send_command(REQUEST_INFO, (char *)"", 0, &remote); //can throw Exception
    Message *net_info = receive_message();
    if (strcmp(net_info->command, PEER_DATA) != 0) {
        throw Exception(PROTOCOL_VIOLATION);
    }
    // TODO: reconfigure peer set
    delete net_info;
}

Daemon::~Daemon() {
    Peer *next = peer_set;
    do {
        Peer *tmp = next;
        next = tmp->next;
        delete tmp;
    } while(next != peer_set);
}

// a REQUEST_INFO message means a new peer is joining the network.
// what the node must do:
// assign the new peer an id (one higher than the current highest id)
// broadcast the peer's data to the other peers
// respond to the initial request with the peer's new id and data about all the other peers
// add it to the peer list, right before the current peer
void Daemon::process_request_info(Message *message) {
}

// a NEW_PEER message means that a new peer is joining the network.
// it has already been received by another node.
// what the node must do:
// add the new peer to the peer list, right before the current peer
void Daemon::process_new_peer(Message *message) {

}
