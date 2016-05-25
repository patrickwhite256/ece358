#include "peer.h"

Peer::Peer(sockaddr_in addr, int count, bool me) {
    address = addr;
    key_count = count;
    is_me = me;
}
