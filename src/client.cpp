#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

#include "basic_exception.h"
#include "client.h"
#include "messages.h"
#include "util.h"

Client::Client(const char *dest_addr, uint16_t dest_port) {
    in_addr addr;
    sockaddr_in dest;
    inet_aton(dest_addr, &addr);
    dest.sin_family = AF_INET;
    dest.sin_addr = addr;
    dest.sin_port = htons(dest_port);

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

    if(connect(sock, (struct sockaddr *)&dest, sizeof(struct sockaddr_in)) < 0) {
        throw Exception(BAD_ADDRESS);
    }
}

Client::~Client() {
    close(sock);
}

void Client::send_command(const char *command, const char *body, uint32_t body_len) {
    size_t msglen = body_len + 9;
    char msg[msglen];
    memcpy(msg, command, 7);

    msg[7] = body_len >> 8;
    msg[8] = body_len - (msg[7] << 8);

    if (body_len > 0) {
        strcpy(&msg[9], body);
    }

    ssize_t sentlen;
    if((sentlen = send(sock, msg, msglen, 0)) < 0 ) {
        die_on_error();
    }
}

void Client::wait_for_ack() {
    unsigned char initial_buffer[INITIAL_BUFFER_LEN];
    ssize_t recv_len = recv(sock, initial_buffer, INITIAL_BUFFER_LEN, 0);
    if(recv_len < 0) {
        die_on_error();
    }

    char cmd[8];
    memcpy(cmd, initial_buffer, 7);
    cmd[7] = '\0';

    if(strcmp(cmd, ACKNOWLEDGE) != 0) {
        throw Exception(PROTOCOL_VIOLATION);
    }
}

char *Client::receive() {
    unsigned char initial_buffer[INITIAL_BUFFER_LEN];
    ssize_t recv_len = recv(sock, initial_buffer, INITIAL_BUFFER_LEN, 0);
    if(recv_len < 0) {
        die_on_error();
    }

    // first 7 bytes reserved for command
    int msg_size = (initial_buffer[7] << 8) + initial_buffer[8];
    char *contents = new char[msg_size + 1];
    int msg_remaining = msg_size;
    int amt_from_first_packet = std::min(msg_remaining, INITIAL_BUFFER_LEN - 9);
    memcpy(contents, &initial_buffer[9], amt_from_first_packet);
    msg_remaining -= amt_from_first_packet;
    while(msg_remaining) {
        recv_len = recv(sock, &contents[msg_size - msg_remaining],
                        msg_remaining, 0);
        msg_remaining -= recv_len;
    }
    contents[msg_size] = '\0';

    return contents;
}
