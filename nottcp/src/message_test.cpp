#include <cstring>
#include <cassert>
#include <string>
#include <iostream>

#include "message.h"

int main(int argc, char* argv[]) {
    Message m("This is some test content", 26, FLAG_ACK);

    unsigned char *serialized = m.serialize();

    std::cout << "Message checksum: " << m.checksum << " Message size: " << m.size << std::endl;

    Message *decoded = Message::deserialize(serialized, m.size);

    std::cout << "Decoded message checksum: " << decoded->checksum <<
                 " Decoded message size: " << decoded->size <<
                 " Decoded content: " << decoded->content << std::endl;

    delete[] serialized;

    decoded->validate();
    delete decoded;
}
