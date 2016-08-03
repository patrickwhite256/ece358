#ifndef MESSAGE_H
#define MESSAGE_H

#define FLAG_ACK 0x01 // 0000 0001 -- acknowledgement of a message
#define FLAG_SYN 0x02 // 0000 0010 -- synchronize request
#define FLAG_FIN 0x04 // 0000 0100 -- close request
#define FLAG_SQN 0x40 // 0100 0000 -- sequence number
#define FLAG_AKN 0x80 // 1000 0000 -- ack sequence number

#define CHECKSUM_SIZE   2 // 16 bits
#define SIZE_SIZE       2 // 16 bits
#define FLAGS_SIZE      1 // 8 bits
#define RAND_SIZE       1 // 8 bits
#define SEQN_SIZE       1 // 8 bits
#define ACKN_SIZE       1 // 8 bits

#define HEADER_SIZE (CHECKSUM_SIZE + SIZE_SIZE + FLAGS_SIZE + RAND_SIZE + SEQN_SIZE + ACKN_SIZE)

#define CHECKSUM_OFFSET 0
#define SIZE_OFFSET     (CHECKSUM_OFFSET + CHECKSUM_SIZE)
#define FLAGS_OFFSET    (SIZE_OFFSET     + SIZE_SIZE)
#define RAND_OFFSET     (FLAGS_OFFSET    + FLAGS_SIZE)
#define SEQN_OFFSET     (RAND_OFFSET     + RAND_SIZE)
#define ACKN_OFFSET     (SEQN_OFFSET     + SEQN_SIZE)

#include <cstdint>
#include <cstdlib>
#include <netinet/in.h>

#include "rcs_exception.h"


/**
 * HEADER STRUCTURE
 *  -------------------------
 * |    16-bit checksum      |
 * |-------------------------|
 * |   16-bit message size   |
 * |-------------------------|
 * | 8 flag bits | random #  |
 *  -------------------------
 * | seq #       | ack #     |
 *  -------------------------
 */

struct Message {
    uint16_t checksum;
    uint8_t flags;
    uint8_t *header;
    uint8_t random;
    char *content;
    uint16_t size;
    uint8_t seq_n;
    uint8_t ack_n;

    sockaddr_in *sender = NULL;

    Message(const char *msg_content, uint16_t content_size, uint8_t flags);
    ~Message();

    uint16_t get_content_size();
    void set_header();
    void set_checksum();
    uint8_t *serialize();
    void validate();

    bool is_ack();
    bool is_syn();
    bool is_fin();
    uint8_t get_sqn();
    uint8_t get_akn();
    void set_sqn(uint8_t sqn);
    void set_akn(uint8_t akn);

    static Message *deserialize(const uint8_t *buf, uint16_t buf_len);
};

#endif
