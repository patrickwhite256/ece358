#include <cstring>
#include <cassert>
#include <string>
#include <iostream>

#include "message.h"

int main(int argc, char* argv[]) {
    Message m("This is some test content", 26, FLAG_ACK, 100);

    unsigned char *serialized = m.serialize();

    std::cout << "Message checksum: " << m.checksum << " Message size: " << m.size << std::endl;
    std::cout << "Message portno: " << +m.portno << std::endl;

    Message decoded = deserialize(serialized);

    std::cout << "Decoded message checksum: " << decoded.checksum <<
                 " Decoded message size: " << decoded.size <<
                 " Decoded portno: " << +decoded.portno <<
                 " Decoded content: " << decoded.content << std::endl;

    delete[] serialized;

    assert(decoded.validate());
}
