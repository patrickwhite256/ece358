#include "peer.h"

class Daemon {
    int id;

    void send_command(char *cmd_id, char *cmd_body, int body_len, sockaddr_in *dest);
    void broadcast(char *cmd_id, char *cmd_body, int body_len, Peer *start);

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
    void loop(int id, int sockfd);
};

void die_on_error();
