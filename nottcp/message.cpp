#include <cstring>
#include "message.h"

Message::Message(char *msg_content, unsigned short content_size, char flags) {
    content = new char[content_size];
    memcpy(content, msg_content, content_size);
    flags = flags;
    size = HEADER_SIZE + content_size;
}

unsigned short Message::get_content_size() {
    return size - HEADER_SIZE;
}

/**
 * set_header - void
 *  Sets this message's header. The checksum field will be initialized to 0 by this method
 */

void Message::set_header() {
    header = new char[HEADER_SIZE];
    unsigned short content_size = get_content_size();

    for (int i = 0; i < CHECKSUM_SIZE; ++i) {
        header[i] = (char)0;
    }

    header[CHECKSUM_SIZE] = content_size >> 8;
    header[CHECKSUM_SIZE + 1] = content_size - (content_size >> 8);

    header[CHECKSUM_SIZE + SIZE_SIZE] = flags;
}

/**
 * set_checksum - void
 *  Set this message's checksum and inserts it into the header.
 *  Don't call this if the checksum is already set. Ideally this
 *  is only to be called immidiately after set_header.
 */

void Message::set_checksum() {
    unsigned short padding_size = 16 - (size % 16);
    unsigned short padded_size = size + padding_size;
    char *checksum_buf = new char[padded_size];

    memcpy(checksum_buf, header, HEADER_SIZE);
    memcpy(&checksum_buf[HEADER_SIZE + 1], content, get_content_size());

    unsigned int checksum_accumulator = 0;

    for (unsigned short i = 0; i < padded_size; i += 16) {
        unsigned short chunk = (checksum_buf[i] << 8) + checksum_buf[i+1];
        checksum_accumulator += chunk;
    }

    unsigned short remainder = checksum_accumulator % 65535;
    checksum = ~((unsigned short)checksum_accumulator + remainder);

    header[0] = (checksum >> 8);
    header[1] = checksum - (checksum >> 8);

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
    memcpy(buf, header, HEADER_SIZE);
    memcpy(&buf[HEADER_SIZE + 1], content, get_content_size());

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
    memcpy(&buf[HEADER_SIZE + 1], content, get_content_size());

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

}

