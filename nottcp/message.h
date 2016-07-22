#ifndef MESSAGE_H
#define MESSAGE_H

#define FLAG_ACK 1 // 0000 0001 -- acknowledgement of a message
#define FLAG_CON 2 // 0000 0010 -- connection request

#define CHECKSUM_SIZE   2 // 16 bits
#define SIZE_SIZE       2 // 16 bits
#define FLAGS_SIZE      1 // 8 bits (probably unnecessary, but easier to work with if it's just a char)

#define HEADER_SIZE (CHECKSUM_SIZE + SIZE_SIZE + FLAGS_SIZE)

#define CHECKSUM_OFFSET 0
#define SIZE_OFFSET     2
#define FLAGS_OFFSET    4


/**
 * HEADER STRUCTURE
 *  -------------------------
 * |    16-bit checksum      |
 * |-------------------------|
 * |   16-bit message size   |
 * |-------------------------|
 * | 8 flag bits |
 *  -------------
 */

struct Message {
    unsigned short checksum;
    char flags;
    char *header;
    char *content;
    unsigned short size;

    Message(char *msg_content, unsigned short content_size, char flags);

    unsigned short get_content_size();
    void set_header();
    void set_checksum();
    char *serialize();
    bool validate();
};

Message deserialize(char *buf);

#endif
