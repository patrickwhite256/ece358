#ifndef DAEMON_H
#define DAEMON_H

#include <vector>
#include <netinet/in.h>
#include <unistd.h>

#include "peer.h"

class Daemon {
    struct Message {
        char *command;
        int size;
        char *body;
        sockaddr_in client;
        int connection;
        Message(char *command, int size, char *body,
                sockaddr_in client, int sockfd) :
            command(command), size(size), body(body),
            client(client), connection(sockfd) {}
        ~Message() {
            close(connection);
            delete command;
            delete body;
        }
    };

    int sockfd;
    int peer_id;
    Peer *peer_set;
    bool terminated;

    // Methods that broadcast messages
    void broadcast_tick_fwd();
    void broadcast_tick_back();
    void broadcast_update_total(int total);
    std::vector<int> broadcast_remove_key(int key);
    std::vector<int> broadcast_get_key(int key);
    int send_add_key(Peer *dest, int key, const char *val);
    void send_content_response(Peer *dest, const char* content);
    int send_add_content(Peer *dest, const char* content);
    void send_key_response(Peer *dest, int key);
    void send_no_key(Peer *dest);

    // Methods that process messages
    void process_tick_fwd();
    void process_tick_back();
    void process_add_key(Message *message);
    void process_remove_key(Message *message);
    void process_get_key(Message *message);
    void process_add_peer(Message *message);
    void process_peer_removal(Message *message);
    void process_update_total(Message *message);
    void process_request_info(Message *message);
    void process_new_peer(Message *message);
    void process_peer_data(Message *message);
    void process_content_response(Message *message);
    void proccess_add_content(Message *message);
    void process_key_response(Message *message);
    void process_no_key(Message *message);
    void process_allkeys(Message *message);

    // Methods that process messages from the client
    void process_client_remove_peer(Message *message);
    void process_client_add_content(Message *message);
    void process_client_remove_content(Message *message);
    void process_client_lookup_content(Message *message);

    //utilites
    Peer *me();
    int send_command(const char *cmd_id, const char *cmd_body, int body_len,
                     sockaddr_in *dest, int sock = -1);
    std::vector<int> broadcast(const char *cmd_id, const char *cmd_body, int body_len);
    Message *receive_message(int sock = -1);
    void close_all(std::vector<int> fd_list);
    std::vector<Message*> wait_for_all(std::vector<int> fd_list);
    Peer *find_peer_by_addr(const char *addr, unsigned short sin_port);
    void wait_for_acks(std::vector<int> fd_list);

    //debug
    void print_peers();

  public:
    Daemon(int sockfd, sockaddr_in server_addr);
    ~Daemon();
    void connect(const char *remote_ip, unsigned short remote_port);
    void loop();
};

#endif
