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

const int INITIAL_BUFFER_LEN = 300;

void die_on_error() {
    std::perror("error:");
    exit(-1);
}

void Daemon::loop(int sockfd) {
    unsigned char initial_buffer[INITIAL_BUFFER_LEN];
    if(listen(sockfd, 0) < 0) {
        die_on_error();
    }

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
