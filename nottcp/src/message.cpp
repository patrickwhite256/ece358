#include <cstring>
#include <iostream>

#include "message.h"

Message::Message(const char *msg_content, uint16_t content_size, uint8_t msg_flags, uint8_t port_no) {
    content = new char[content_size];
    memcpy(content, msg_content, content_size);
    flags = msg_flags;
    size = HEADER_SIZE + content_size;
    portno = port_no;
    header = NULL;
}

uint16_t Message::get_content_size() {
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

    header = new uint8_t[HEADER_SIZE];

    for (uint8_t i = CHECKSUM_OFFSET; i < CHECKSUM_SIZE; ++i) {
        header[i] = (char)0;
    }

    header[SIZE_OFFSET] = size >> 8;
    header[SIZE_OFFSET + 1] = size & 0xff;

    header[FLAGS_OFFSET] = flags;
    header[PNO_OFFSET] = portno;
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

    uint16_t padding_size = size % 2;
    uint16_t padded_size = size + padding_size;
    uint8_t *checksum_buf = new uint8_t[padded_size];

    memcpy(checksum_buf, header, HEADER_SIZE);
    memcpy(&checksum_buf[HEADER_SIZE], content, get_content_size());

    for (uint16_t i = size; i < padded_size; ++i) {
        checksum_buf[i] = 0;
    }

    uint32_t checksum_accumulator = 0;

    for (uint16_t i = 0; i < padded_size; i += 2) {
        uint16_t chunk = (checksum_buf[i] << 8) + checksum_buf[i+1];
        checksum_accumulator += chunk;
    }

    while(checksum_accumulator > 0xffff) {
        checksum_accumulator = (checksum_accumulator >> 16) + (checksum_accumulator & 0xffff);
    }
    checksum = ~checksum_accumulator;

    header[CHECKSUM_OFFSET] = (checksum >> 8);
    header[CHECKSUM_OFFSET + 1] = checksum & 0xff;

    delete[] checksum_buf;
}

/**
 * serialize - char*
 *  Returns a char buffer containing this message's header and content,
 *  ready to be shipped off by the network protocol
 */

uint8_t *Message::serialize() {
    uint8_t *buf = new uint8_t[size];

    set_header();
    set_checksum();
    memcpy(buf, header, HEADER_SIZE);

    memcpy(&buf[HEADER_SIZE], content, get_content_size());

    return buf;
}


/**
 * validate - bool
 *  Uses the current checksum value to check if this message is corrupt.
 *  Returns true if the message is intact, false if corrupt.
 */
bool Message::validate() {
    uint16_t padding_size = size % 2;
    uint16_t padded_size = size + padding_size;
    uint8_t *buf = new uint8_t[padded_size];

    set_header();
    header[CHECKSUM_OFFSET] = checksum >> 8;
    header[CHECKSUM_OFFSET + 1] = checksum & 0xff;
    memcpy(buf, header, HEADER_SIZE);
    memcpy(&buf[HEADER_SIZE], content, get_content_size());

    for (uint16_t i = size; i < padded_size; ++i) {
        buf[i] = 0;
    }

    uint32_t checksum_accumulator = 0;

    for (uint16_t i = 0; i < padded_size; i += 2) {
        uint16_t chunk = (buf[i] << 8) + buf[i+1];
        checksum_accumulator += chunk;
    }

    while(checksum_accumulator > 0xffff) {
        checksum_accumulator = (checksum_accumulator >> 16) + (checksum_accumulator & 0xffff);
    }

    uint32_t complement = ~checksum_accumulator & 0xffff;
    return complement == 0;
}

/**
 * deserialize - Message
 *  Accepts a character buf and decomposes it into a Message object
 */
Message deserialize(const uint8_t *buf) {
    uint16_t checksum = (buf[CHECKSUM_OFFSET] << 8) + buf[CHECKSUM_OFFSET + 1];
    uint8_t flags = buf[FLAGS_OFFSET];
    uint8_t portno = buf[PNO_OFFSET];
    uint16_t size = (buf[SIZE_OFFSET] << 8) + buf[SIZE_OFFSET + 1];

    char *content;
    uint16_t content_size = size - HEADER_SIZE;

    content = new char[content_size];
    memcpy(content, &buf[HEADER_SIZE], content_size);

    Message ret(content, content_size, flags, portno);
    ret.checksum = checksum;

    delete[] content;

    return ret;
}

