#ifndef PEER_H
#define PEER_H

#include <netinet/in.h>

struct Peer {
    Peer *next;
    Peer *previous;
    unsigned int key_count;
    int id;
    sockaddr_in address;

    Peer(sockaddr_in address, unsigned int key_count, int id);
};

#endif
