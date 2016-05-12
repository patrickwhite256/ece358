#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <algorithm>

const int INITIAL_BUFFER_LEN = 300;

int pickServerIPAddr(in_addr *srv_ip);

void handle_error() {
    std::cout << "errrrrrrrr" << std::endl;
}

void daemon_loop(int sockfd) {
    char initial_buffer[INITIAL_BUFFER_LEN];
    if(listen(sockfd, 0) < 0) {
        handle_error();
    }

    while(true) {
        sockaddr_in client;
        socklen_t alen = sizeof(sockaddr_in);
        int connectedsock = accept(sockfd, (sockaddr *)&client, &alen);
        if(connectedsock < 0) {
            handle_error();
        }
        ssize_t recv_len = recv(connectedsock, initial_buffer,
                                INITIAL_BUFFER_LEN - 1, 0);
        if(recv_len < 0) {
            handle_error();
        }

        char cmd[8];
        memcpy(cmd, initial_buffer, 7);
        cmd[7] = '\0';

        /* std::cout << "msg received: " << initial_buffer << std::endl; */

        if(strcmp(cmd, "allkeys") == 0) {

            std::cout << "allkeys!" << std::endl;
        } else {
            int msg_size = (initial_buffer[7] << 8) + initial_buffer[8];
            char *contents = new char[msg_size];
            std::cout << "size: " << msg_size << std::endl;
            int msg_remaining = msg_size - (recv_len - 9);
            int amt_from_first_packet = std::min(msg_remaining, INITIAL_BUFFER_LEN - 9);
            memcpy(contents, &initial_buffer[9], amt_from_first_packet);
            msg_remaining -= amt_from_first_packet;
            while(msg_remaining) {
                recv_len = recv(connectedsock, &contents[msg_size - msg_remaining],
                                msg_remaining, 0);
            }
        }
    }
}

int main(int argc, char **argv) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        handle_error();
    }

    in_addr srv_ip;

    pickServerIPAddr(&srv_ip);

    sockaddr_in server;
    server.sin_family = AF_INET;
    memcpy(&(server.sin_addr), &srv_ip, sizeof(sockaddr_in));
    // somehow set sin_addr
    server.sin_port = 0;

    if(bind(sockfd, (sockaddr *)&server, sizeof(sockaddr_in)) < 0) {
        handle_error();
    }
    socklen_t alen = sizeof(sockaddr_in);
    if(getsockname(sockfd, (sockaddr*)&server, &alen) < 0) {
        handle_error();
    }

    std::cout << "server at" << inet_ntoa(server.sin_addr) << ":"
         << ntohs(server.sin_port) << std::endl;

    daemon_loop(sockfd);
}
