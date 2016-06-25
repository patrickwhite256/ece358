#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

class Client {
    int sock;
    public:
        Client(const char *dest_addr, uint16_t dest_port);
        ~Client();
        void send_command(const char *command, const char *body, uint32_t bodylen);
        char *receive();
        void wait_for_ack();
};

#endif
