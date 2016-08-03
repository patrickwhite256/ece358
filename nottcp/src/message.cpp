#include <cstring>
#include <iostream>

#include "message.h"

// Calculates a 16-bit 1's complement checksum over an arbitrary byte buffer
uint16_t compute_checksum(uint8_t *buf, uint16_t size) {
    uint16_t padding_size = size % 2;
    uint16_t padded_size = size + padding_size;
    uint8_t *checksum_buf = new uint8_t[padded_size];

    memcpy(checksum_buf, buf, size);

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

    delete[] checksum_buf;
    return (uint16_t)(~checksum_accumulator & 0xffff);
}


Message::Message(const char *msg_content, uint16_t content_size, uint8_t msg_flags) {
    content = new char[content_size];
    if(msg_content != NULL) {
        memcpy(content, msg_content, content_size);
    }
    flags = msg_flags;
    size = HEADER_SIZE + content_size;

    checksum = 0;
    seq_n = 0;
    ack_n = 0;
    random = rand();
    header = NULL;
}

Message::~Message() {
    delete[] content;
    if(header) delete[] header;
    if(sender) delete sender;
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
    header[RAND_OFFSET] = random;
    header[SEQN_OFFSET] = seq_n;
    header[ACKN_OFFSET] = ack_n;
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

    uint8_t *msg_buf = new uint8_t[size];

    memcpy(msg_buf, header, HEADER_SIZE);
    memcpy(&msg_buf[HEADER_SIZE], content, get_content_size());

    checksum = compute_checksum(msg_buf, size);

    delete[] msg_buf;

    header[CHECKSUM_OFFSET] = (checksum >> 8);
    header[CHECKSUM_OFFSET + 1] = checksum & 0xff;
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
void Message::validate() {
    uint8_t *buf = new uint8_t[size];

    set_header();
    header[CHECKSUM_OFFSET] = checksum >> 8;
    header[CHECKSUM_OFFSET + 1] = checksum & 0xff;

    memcpy(buf, header, HEADER_SIZE);
    memcpy(&buf[HEADER_SIZE], content, get_content_size());

    uint16_t computed_checksum = compute_checksum(buf, size);

    delete[] buf;

    if(computed_checksum != 0) {
        throw RCSException(RCS_ERROR_CORRUPT);
    }
}

bool Message::is_ack() {
    return (flags & FLAG_ACK) > 0;
}

bool Message::is_syn() {
    return (flags & FLAG_SYN) > 0;
}

bool Message::is_fin() {
    return (flags & FLAG_FIN) > 0;
}

uint8_t Message::get_sqn() {
    return seq_n;
}

uint8_t Message::get_akn() {
    return ack_n;
}

void Message::set_sqn(uint8_t sqn) {
    seq_n = sqn;
}

void Message::set_akn(uint8_t akn) {
    ack_n = akn;
}

/**
 * deserialize - Message
 *  Accepts a character buf and decomposes it into a Message object
 *  May throw RCSException if the message is invalid.
 */
Message *Message::deserialize(const uint8_t *buf, uint16_t buf_len) {
    if(buf_len < HEADER_SIZE) {
        throw RCSException(RCS_ERROR_CORRUPT);
    }

    uint16_t checksum = (buf[CHECKSUM_OFFSET] << 8) + buf[CHECKSUM_OFFSET + 1];
    uint8_t flags = buf[FLAGS_OFFSET];
    uint8_t random = buf[RAND_OFFSET];
    uint8_t sqn = buf[SEQN_OFFSET];
    uint8_t akn = buf[ACKN_OFFSET];
    uint16_t size = (buf[SIZE_OFFSET] << 8) + buf[SIZE_OFFSET + 1];
    if(buf_len < size) {
        throw RCSException(RCS_ERROR_CORRUPT);
    }

    uint16_t content_size = size - HEADER_SIZE;
    char *content = new char[content_size];
    memcpy(content, &buf[HEADER_SIZE], content_size);

    Message *ret = new Message(content, content_size, flags);
    ret->checksum = checksum;
    ret->random = random;
    ret->seq_n = sqn;
    ret->ack_n = akn;

    delete[] content;

    return ret;
}
