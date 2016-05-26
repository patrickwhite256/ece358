#ifndef PEER_H
#define PEER_H

#include <netinet/in.h>

struct Peer {
    Peer *next;
    Peer *previous;
    int key_count;
    bool is_me;
    sockaddr_in address;

    Peer(sockaddr_in addr, int count, bool me = false);
};

#endif
