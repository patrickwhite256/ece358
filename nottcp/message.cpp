#include <cstring>
#include <iostream>

#include "message.h"

Message::Message(char *msg_content, unsigned short content_size, char flags) {
    content = new char[content_size];
    std::cout << "constructing with content " << msg_content << std::endl;
    memcpy(content, msg_content, content_size);
    flags = flags;
    size = HEADER_SIZE + content_size;
    header = NULL;
}

unsigned short Message::get_content_size() {
    return size - HEADER_SIZE;
}

/**
 * set_header - void
 *  Sets this message's header. The checksum field will be initialized to 0 by this method
 */

void Message::set_header() {
    if (NULL != header) {
        delete[] header;
    }

    header = new char[HEADER_SIZE];

    for (int i = CHECKSUM_OFFSET; i < CHECKSUM_SIZE; ++i) {
        header[i] = (char)0;
    }

    header[SIZE_OFFSET] = size >> 8;
    header[SIZE_OFFSET + 1] = size - (header[SIZE_OFFSET] << 8);

    header[FLAGS_OFFSET] = flags;
}

/**
 * set_checksum - void
 *  Set this message's checksum and inserts it into the header.
 *  Don't call this if the checksum is already set. Ideally this
 *  is only to be called immidiately after set_header.
 */

void Message::set_checksum() {
    if (NULL == header) {
        return;
    }

    unsigned short padding_size = 16 - (size % 16);
    unsigned short padded_size = size + padding_size;
    char *checksum_buf = new char[padded_size];

    memcpy(checksum_buf, header, HEADER_SIZE);
    memcpy(&checksum_buf[HEADER_SIZE], content, get_content_size());

    for (int i = get_content_size(); i < padded_size; ++i) {
        checksum_buf[i] = 0;
    }

    unsigned int checksum_accumulator = 0;

    for (unsigned short i = 0; i < padded_size; i += 16) {
        unsigned short chunk = (checksum_buf[i] << 8) + checksum_buf[i+1];
        checksum_accumulator += chunk;
    }

    unsigned short remainder = checksum_accumulator % 65535;
    checksum = ~((unsigned short)checksum_accumulator + remainder);

    std::cout << "checksum " << checksum << std::endl;

    header[CHECKSUM_OFFSET] = (checksum >> 8);
    header[CHECKSUM_OFFSET + 1] = checksum - ((unsigned short)header[CHECKSUM_OFFSET] << 8);
    std::cout << ((unsigned short)header[CHECKSUM_OFFSET] << 8) << std::endl;
    std::cout << "First byte of checksum " << (unsigned short)header[CHECKSUM_OFFSET] << " second byte of checksum " << (unsigned short)header[CHECKSUM_OFFSET + 1] << std::endl;

    delete[] checksum_buf;
}

/**
 * serialize - char*
 *  Returns a char buffer containing this message's header and content,
 *  ready to be shipped off by the network protocol
 */

char *Message::serialize() {
    char *buf = new char[size];

    set_header();
    set_checksum();
    memcpy(buf, header, HEADER_SIZE);

    std::cout << get_content_size() << std::endl;
    std::cout << "writing content " << content << std::endl;
    memcpy(&buf[HEADER_SIZE], content, get_content_size());

    return buf;
}


/**
 * validate - bool
 *  Uses the current checksum value to check if this message is corrupt.
 *  Returns true if the message is intact, false if corrupt.
 */
bool Message::validate() {
    unsigned short padding_size = 16 - (size % 16);
    unsigned short padded_size = size + padding_size;
    char *buf = new char[padded_size];

    memcpy(buf, header, HEADER_SIZE);
    memcpy(&buf[HEADER_SIZE], content, get_content_size());

    for (int i = get_content_size(); i < padded_size; ++i) {
        buf[i] = 0;
    }

    unsigned short checksum_accumulator = 0;

    for (unsigned short i = 0; i < padded_size; i += 16) {
        unsigned short chunk = (buf[i] << 8) + buf[i+1];
        checksum_accumulator += chunk;
    }

    unsigned short complement = ~checksum_accumulator;
    return complement == 0;
}

/**
 * deserialize - Message
 *  Accepts a character buf and decomposes it into a Message object
 */
Message deserialize(char *buf) {
    unsigned short checksum = (buf[CHECKSUM_OFFSET] << 8) + buf[CHECKSUM_OFFSET + 1];
    std::cout << "Checksum: " << checksum << std::endl;
    char flags = buf[FLAGS_OFFSET];
    std::cout << "Flags: " << (int)flags << std::endl;
    unsigned short size = (buf[SIZE_OFFSET << 8]) + buf[SIZE_OFFSET + 1];
    std::cout << "Size: " << size << std::endl;

    char *content;
    unsigned short content_size = size - HEADER_SIZE;

    content = new char[content_size];
    memcpy(content, &buf[HEADER_SIZE], content_size);

    Message ret(content, content_size, flags);
    ret.checksum = checksum;

    delete[] content;

    return ret;
}

