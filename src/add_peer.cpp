#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>

#include "daemon.h"

int main(int argc, char **argv) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        die_on_error();
    }

    in_addr srv_ip;

    ifaddrs *ifa;
    if(getifaddrs(&ifa) < 0) {
        die_on_error();
    }

    for(ifaddrs *i = ifa; i != NULL; i = i->ifa_next) {
        sockaddr *addr = i->ifa_addr;
        if(addr == NULL) continue;
        if(addr->sa_family == AF_INET &&
           (i->ifa_flags & IFF_LOOPBACK) == 0) {
            memcpy(&srv_ip,
                   &(((sockaddr_in *)addr)->sin_addr),
                   sizeof(in_addr));
            break;
        }
    }

    freeifaddrs(ifa);

    sockaddr_in server;
    server.sin_family = AF_INET;
    memcpy(&(server.sin_addr), &srv_ip, sizeof(sockaddr_in));
    server.sin_port = 0;

    if(bind(sockfd, (sockaddr *)&server, sizeof(sockaddr_in)) < 0) {
        die_on_error();
    }

    socklen_t alen = sizeof(sockaddr_in);
    if(getsockname(sockfd, (sockaddr*)&server, &alen) < 0) {
        die_on_error();
    }

    std::cout << inet_ntoa(server.sin_addr)
              << " "
              << ntohs(server.sin_port)
              << std::endl;

    if(daemon(0, 0) < 0) { // fork process
        die_on_error();
    }

    if (argc == 1) { //first peer
        Daemon dmon;
        dmon.loop(sockfd);
    } else { //connect to peer
    }
}