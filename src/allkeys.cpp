#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include "util.h"


int main(int argc, char **argv) {
    in_addr addr;
    sockaddr_in dest;
    inet_aton(argv[1], &addr);
    dest.sin_family = AF_INET;
    dest.sin_addr = addr;
    dest.sin_port = htons(atoi(argv[2]));

    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        die_on_error();
    }

    sockaddr_in client;
    bzero(&client, sizeof(struct sockaddr_in));
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_port = 0;
    if(bind(sock, (struct sockaddr *)&client, sizeof(struct sockaddr_in)) < 0) {
        die_on_error();
    }

    socklen_t alen = sizeof(struct sockaddr_in);
    if(getsockname(sock, (struct sockaddr *)&client, &alen) < 0) {
        die_on_error();
    }

    if(::connect(sock, (struct sockaddr *)&dest, sizeof(struct sockaddr_in)) < 0) {
        die_on_error();
    }

    if((send(sock, "allkeys", 8, 0)) < 0 ) {
        die_on_error();
    }

    char allkeys_buffer[100];
    int recvlen;
    if((recvlen = recv(sock, allkeys_buffer, 100, 0)) < 0) {
        die_on_error();
    }
    std::cout << allkeys_buffer << std::endl;

    close(sock);
}
