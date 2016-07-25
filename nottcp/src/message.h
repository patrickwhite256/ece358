#ifndef MESSAGE_H
#define MESSAGE_H

#define FLAG_ACK 0x01 // 0000 0001 -- acknowledgement of a message
#define FLAG_SYN 0x02 // 0000 0010 -- synchronize request
#define FLAG_SQN 0x40 // 0100 0000 -- sequence number
#define FLAG_AKN 0x80 // 1000 0000 -- ack sequence number

#define CHECKSUM_SIZE   2 // 16 bits
#define SIZE_SIZE       2 // 16 bits
#define FLAGS_SIZE      1 // 8 bits
#define PNO_SIZE        1 // 8 bits

#define HEADER_SIZE (CHECKSUM_SIZE + SIZE_SIZE + FLAGS_SIZE + PNO_SIZE)

#define CHECKSUM_OFFSET 0
#define SIZE_OFFSET     (CHECKSUM_OFFSET + CHECKSUM_SIZE)
#define FLAGS_OFFSET    (SIZE_OFFSET     + SIZE_SIZE)
#define PNO_OFFSET      (FLAGS_OFFSET    + FLAGS_SIZE)

#include <cstdint>


/**
 * HEADER STRUCTURE
 *  -------------------------
 * |    16-bit checksum      |
 * |-------------------------|
 * |   16-bit message size   |
 * |-------------------------|
 * | 8 flag bits | "port" #  |
 *  -------------------------
 */

struct Message {
    uint16_t checksum;
    uint8_t flags;
    uint8_t portno;
    uint8_t *header;
    char *content;
    uint16_t size;

    Message(const char *msg_content, uint16_t content_size, uint8_t flags, uint8_t portno);

    uint16_t get_content_size();
    void set_header();
    void set_checksum();
    uint8_t *serialize();
    bool validate();
};

Message deserialize(const uint8_t *buf);

#endif
