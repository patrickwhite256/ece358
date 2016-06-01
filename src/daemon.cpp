#include <iostream>
#include <cstring>
#include <algorithm>
#include <arpa/inet.h>
#include <cstdio>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "daemon.h"
#include "peer.h"

const int INITIAL_BUFFER_LEN = 300;

void die_on_error() {
    std::perror("error:");
    exit(-1);
}

void Daemon::loop(int id, int sockfd) {
    unsigned char initial_buffer[INITIAL_BUFFER_LEN];
    if(listen(sockfd, 0) < 0) {
        die_on_error();
    }

    this->id = id;

    while(true) {
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

        char cmd[8];
        memcpy(cmd, initial_buffer, 7);
        cmd[7] = '\0';

        if(strcmp(cmd, "allkeys") == 0) {
            // do the allkeys thing
        } else {
            int msg_size = (initial_buffer[7] << 8) + initial_buffer[8];
            unsigned char *contents = new unsigned char[msg_size + 1];
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

            if(strcmp(cmd, "addakey") == 0) {
                //etc
            }
        }
    }
}

void Daemon::send_command(char *cmd_id, char *cmd_body, int body_len, sockaddr_in *dest) {
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

    if(connect(sockfd, (struct sockaddr *)dest, sizeof(struct sockaddr_in)) < 0) {
        die_on_error();
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

void Daemon::broadcast(char *cmd_id, char *cmd_body, int body_len, Peer *start) {
    if (start->id != this->id) {
        send_command(cmd_id, cmd_body, body_len, &start->address);
    }

    Peer *next = start->next;

    while(next != start) {
        if (next->id == this->id) continue;

        send_command(cmd_id, cmd_body, body_len, &next->address);
        next = next->next;
    }
}

