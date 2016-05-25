#ifndef PEER_H
#define PEER_H

#include <netinet/in.h>

class Peer {
    Peer *next;
    Peer *previous;
    int key_count;
    bool is_me;
    sockaddr_in address;
  public:
    Peer(sockaddr_in addr, int count, bool me = false);
};

#endif
