#include <arpa/inet.h>
#include <cstring>
#include <cstdlib>
#include <ifaddrs.h>
#include <iostream>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>

#include "util.h"
#include "daemon.h"
#include "basic_exception.h"
#include "mybind.h"

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

    if(mybind(sockfd, &server) < 0) {
        die_on_error();
    }

    socklen_t alen = sizeof(sockaddr_in);
    if(getsockname(sockfd, (sockaddr*)&server, &alen) < 0) {
        die_on_error();
    }

    Daemon dmon(sockfd, server);
    if (argc > 1) { //connect to peer
        if (argc != 3) {
            std::cerr << "invalid arguments" << std::endl;
            exit(-1);
        }
        try {
            dmon.connect(argv[1], htons(atoi(argv[2])));
        } catch (Exception ex) {
            std::cerr << "Error: no such peer" << std::endl;
            exit(-1);
        }
    }

    std::cout << inet_ntoa(server.sin_addr)
              << " "
              << ntohs(server.sin_port)
              << std::endl;

#ifndef DEBUG
    if(daemon(0, 1) < 0) { // fork process
        die_on_error();
    }
#endif

    dmon.loop();
}
