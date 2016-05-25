#include "peer.h"

Peer::Peer(sockaddr_in address, int key_count, int id) {
    this->address   = address;
    this->key_count = key_count;
    this->id        = id;
}

void PeerDaemon::send_command(char *cmd_id, char *cmd_body, int body_len, sockaddr_in dest) {
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

    if(connect(sockfd, (struct sockaddr *)&dest, sizeof(struct sockaddr_in)) < 0) {
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

void PeerDaemon::broadcast(char *cmd_id, char *cmd_body, int body_len, Peer *start) {
    if (!start->me) {
        send_command(cmd_id, cmd_body, body_len, start->addr);
    }

    Peer *next = start->left;

    while(next != start) {
        if (next->me) continue;

        send_command(cmd_id, cmd_body, body_len, next->addr);
        next = next->left;
    }
}

