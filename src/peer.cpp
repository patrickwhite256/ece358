#include "peer.h"

Peer::Peer(sockaddr_in address, unsigned int key_count, int id) {
    this->address   = address;
    this->key_count = key_count;
    this->id        = id;
}
