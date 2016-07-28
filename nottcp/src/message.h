#ifndef MESSAGE_H
#define MESSAGE_H

#define FLAG_ACK 0x01 // 0000 0001 -- acknowledgement of a message
#define FLAG_SYN 0x02 // 0000 0010 -- synchronize request
#define FLAG_SQN 0x40 // 0100 0000 -- sequence number
#define FLAG_AKN 0x80 // 1000 0000 -- ack sequence number

#define CHECKSUM_SIZE   2 // 16 bits
#define SIZE_SIZE       2 // 16 bits
#define DPORT_SIZE      1 // 8 bits
#define SPORT_SIZE      1 // 8 bits
#define FLAGS_SIZE      1 // 8 bits

#define HEADER_SIZE (CHECKSUM_SIZE + SIZE_SIZE + DPORT_SIZE + SPORT_SIZE + FLAGS_SIZE)

#define CHECKSUM_OFFSET 0
#define SIZE_OFFSET     (CHECKSUM_OFFSET + CHECKSUM_SIZE)
#define DPORT_OFFSET    (SIZE_OFFSET     + SIZE_SIZE)
#define SPORT_OFFSET    (DPORT_OFFSET    + DPORT_SIZE)
#define FLAGS_OFFSET    (SPORT_OFFSET    + SPORT_SIZE)

#include <cstdint>


/**
 * HEADER STRUCTURE
 *  -------------------------
 * |    16-bit checksum      |
 * |-------------------------|
 * |   16-bit message size   |
 * |-------------------------|
 * |  dest port  | src port  |
 * |-------------------------|
 * | 8 flag bits |           |
 *  -------------------------
 */

struct Message {
    uint16_t checksum;
    uint8_t flags;
    uint8_t s_port = 0;
    uint8_t d_port = 0;
    uint8_t *header;
    char *content;
    uint16_t size;

    Message(const char *msg_content, uint16_t content_size, uint8_t flags);
    ~Message();

    uint16_t get_content_size();
    void set_header();
    void set_checksum();
    uint8_t *serialize();
    bool validate();

    bool is_ack();
    bool is_syn();
    uint8_t get_sqn();
    uint8_t get_akn();
    void set_sqn(uint8_t sqn);
    void set_akn(uint8_t akn);
};

//TODO: static
Message *deserialize(const uint8_t *buf, uint16_t buf_len);

#endif
