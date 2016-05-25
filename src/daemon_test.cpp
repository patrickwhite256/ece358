#include <iostream>
#include <cstring>
#include <arpa/inet.h>

#include "daemon.h"

int pickServerIPAddr(in_addr *srv_ip);

int main(int argc, char **argv) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        die_on_error();
    }

    in_addr srv_ip;

    pickServerIPAddr(&srv_ip);

    sockaddr_in server;
    server.sin_family = AF_INET;
    memcpy(&(server.sin_addr), &srv_ip, sizeof(sockaddr_in));
    // somehow set sin_addr
    server.sin_port = 0;

    if(bind(sockfd, (sockaddr *)&server, sizeof(sockaddr_in)) < 0) {
        die_on_error();
    }
    socklen_t alen = sizeof(sockaddr_in);
    if(getsockname(sockfd, (sockaddr*)&server, &alen) < 0) {
        die_on_error();
    }

    std::cout << "server at" << inet_ntoa(server.sin_addr) << ":"
         << ntohs(server.sin_port) << std::endl;

    Daemon *daemon = new Daemon;
    daemon->loop(sockfd);
    delete daemon;
}
