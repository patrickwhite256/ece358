#include "peer.h"

#include <netinet/in.h>
#include "peer.h"

class Daemon {
    struct Message {
        char *command;
        int size;
        char *body;
        sockaddr_in client;
        Message(char *command, int size, char *body, sockaddr_in client) :
            command(command), size(size), body(body), client(client) {}
        ~Message() {
            delete command;
            delete body;
        }
    };

    int sockfd;
    int peer_id;
    Peer *peer_set;

    void send_command(const char *cmd_id, char *cmd_body, int body_len, sockaddr_in *dest);
    void broadcast(char *cmd_id, char *cmd_body, int body_len, Peer *start);
    Message *receive_message();

    // Methods that broadcast messages
    void broadcast_tick_fwd();
    void broadcast_tick_back();
    void broadcast_update_total(int total);
    void broadcast_remove_key(int key);
    void send_add_key(int dest_id);

    // Methods that process messages
    void process_tick_fwd();
    void process_tick_back();
    void process_add_key(char *body);
    void process_remove_key(char *body);
    void process_add_peer(char *body);
    void process_remove_peer(char *body);
    void process_update_total(char *body);

  public:
    Daemon(int sockfd);
    void connect(const char *remote_ip, int remote_port);
    void loop();
};

void die_on_error();
