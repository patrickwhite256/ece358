#include <cstring>
#include <cmath>
#include <algorithm>
#include <arpa/inet.h>
#include <iostream>
#include <netinet/tcp.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>

#include "basic_exception.h"
#include "daemon.h"
#include "messages.h"
#include "peer.h"
#include "util.h"

// messages that can be heard while listening
const char *REQUEST_INFO     = "reqinfo";
const char *NEW_PEER         = "newpeer";
const char *TICK_FWD         = "tickfwd";
const char *TICK_BACK        = "tickbac";
const char *UPDATE_TOTALS    = "upd8tot";
const char *REMOVE_KEY       = "remvkey";
const char *GET_KEY          = "getakey";
const char *ADD_CONTENT      = "addcont";
const char *PEER_REMOVAL     = "iamdead";
const char *STEAL_KEY        = "giffkey";
const char *FORCE_KEY        = "takekey";

// messages that are part of protocols
const char *PEER_DATA        = "peerdta";
const char *CONTENT_RESPONSE = "content";
const char *NO_KEY           = "nokey4u";
const char *KEY_RESPONSE     = "keyresp";
const char *REMOVE_RESPONSE  = "dropkey";
const char *STEAL_RESPONSE   = "hereugo";

const char *TOKEN_SEPARATOR     = "\x07";
const char TOKEN_SEPARATOR_CHAR = '\x07';

Daemon::Daemon(int sockfd, sockaddr_in server_addr) {
    this->sockfd = sockfd;
    this->peer_id = 0; // assume we are the first node until told otherwise
    this->key_counter = 0;
    this->id_counter = 0;

    peer_set = new Peer(server_addr, 0, 0);
    peer_set->next = peer_set;
    peer_set->previous = peer_set;
    terminated = false;
}

void Daemon::loop() {
    if(listen(sockfd, 0) < 0) {
        die_on_error();
    }

    while(!terminated) {
        Daemon::Message *message = receive_message();
        if(msg_command_is(message, ALL_KEYS)) {
            process_allkeys(message);
        } else if (msg_command_is(message, REQUEST_INFO)) {
            process_request_info(message);
        } else if (msg_command_is(message, NEW_PEER)) {
            process_new_peer(message);
        } else if (msg_command_is(message, REMOVE_PEER)) {
            process_client_remove_peer(message);
        } else if (msg_command_is(message, PEER_REMOVAL)) {
            process_peer_removal(message);
        } else if (msg_command_is(message, C_ADD_CONTENT)) {
            process_client_add_content(message);
        } else if (msg_command_is(message, ADD_CONTENT)) {
            process_add_content(message);
        } else if (msg_command_is(message, TICK_FWD)) {
            process_tick_fwd(message);
        } else if (msg_command_is(message, TICK_BACK)) {
            process_tick_back(message);
        } else if (msg_command_is(message, UPDATE_TOTALS)) {
            process_update_totals(message);
        } else if (msg_command_is(message, C_LOOKUP_CONTENT)) {
            process_client_lookup_content(message);
        } else if (msg_command_is(message, GET_KEY)) {
            process_get_key(message);
        } else if (msg_command_is(message, STEAL_KEY)) {
            process_steal_key(message);
        } else if (msg_command_is(message, REMOVE_KEY)) {
            process_remove_key(message);
        } else if (msg_command_is(message, C_REMOVE_CONTENT)) {
            process_client_remove_content(message);
        } else if (msg_command_is(message, FORCE_KEY)) {
            process_force_key(message);
        }

        delete message;
    }
}

bool Daemon::msg_command_is(Message *msg, const char *command) {
   return strcmp(msg->command, command) == 0;
}

void Daemon::print_peers() {
#ifdef DEBUG
    Peer *next = peer_set;
    do {
        std::cout << next->id << ") "
                  << inet_ntoa(next->address.sin_addr) << ":"
                  << ntohs(next->address.sin_port)
                  << " Key Count: " << next->key_count << std::endl;
        next = next->next;
    } while (next != peer_set);
#endif
}

void Daemon::print_table() {
#ifdef DEBUG
    std::cout << "Table contents:" << std::endl;
    for (std::map<int, std::string>::iterator it = key_map.begin(); it != key_map.end(); ++it) {
        std::cout << it->first << ": " << it->second << std::endl;
    }
#endif
}

/*
 * receive_message
 *   receives a message and returns it as a Message
 *   @param sock: socket file descriptor on which to receive message,
 *                or -1 to accept a new connection
 */
Daemon::Message *Daemon::receive_message(int sock) {
    unsigned char initial_buffer[INITIAL_BUFFER_LEN];
    sockaddr_in client;
    socklen_t alen = sizeof(sockaddr_in);
    if(sock == -1) {
        sock = accept(sockfd, (sockaddr *)&client, &alen);
#ifdef VERBOSE
        std::cout << "connection from " << inet_ntoa(client.sin_addr)
                  << ":" << ntohs(client.sin_port) << std::endl;
#endif
        if(sock < 0) {
            die_on_error();
        }
    }
    ssize_t recv_len = recv(sock, initial_buffer, INITIAL_BUFFER_LEN, 0);
    if(recv_len < 0) {
        die_on_error();
    }

    char *cmd = new char[8];
    memcpy(cmd, initial_buffer, 7);
    cmd[7] = '\0';
#ifdef VERBOSE
    std::cout << "received " << cmd << std::endl;
#endif

    if(strcmp(cmd, ALL_KEYS) == 0) {
        char *dummy = new char[1];
        return new Daemon::Message(cmd, 0, dummy, client, sock);
    } else {
        int msg_size = (initial_buffer[7] << 8) + initial_buffer[8];
        char *contents = new char[msg_size + 1];
        int msg_remaining = msg_size;
        int amt_from_first_packet = std::min(msg_remaining, INITIAL_BUFFER_LEN - 9);
        memcpy(contents, &initial_buffer[9], amt_from_first_packet);
        msg_remaining -= amt_from_first_packet;
        while(msg_remaining) {
            recv_len = recv(sock, &contents[msg_size - msg_remaining],
                            msg_remaining, 0);
            msg_remaining -= recv_len;
        }
        contents[msg_size] = '\0';

        return new Daemon::Message(cmd, msg_size, contents, client, sock);
    }
}

/*
 * send_command
 *   send a command using the universal format
 *   @param cmd_id   command to send (should be a constant defined here or in
 *                   messages.h
 *   @param cmd_body body of the message to send. can be NULL for no body.
 *   @param body_len length of the body. must be 0 if cmd_body is NULL
 *   @param dest     destination ip/port to send to. if not NULL, will open
 *                   a new connection.
 *   @param sock     socket file descriptor of an open connection to use.
 *                   only used if dest is NULL
 *   @return the socket file descriptor of the connection used
 */
int Daemon::send_command(const char *cmd_id, const char *cmd_body,
                          int body_len, sockaddr_in *dest, int sock) {
    if(dest != NULL) {
        if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            die_on_error();
        }

        struct sockaddr_in client;
        bzero(&client, sizeof(struct sockaddr_in));
        client.sin_family = AF_INET;
        client.sin_addr.s_addr = htonl(INADDR_ANY);
        client.sin_port = 0;
        if(bind(sock, (struct sockaddr *)&client, sizeof(struct sockaddr_in)) < 0) {
            die_on_error();
        }

        socklen_t alen = sizeof(struct sockaddr_in);
        if(getsockname(sock, (struct sockaddr *)&client, &alen) < 0) {
            die_on_error();
        }
        if(::connect(sock, (struct sockaddr *)dest, sizeof(struct sockaddr_in)) < 0) {
            throw Exception(BAD_ADDRESS);
        }

    }
    size_t msglen = body_len + 9;
    char msg[msglen];
    memcpy(msg, cmd_id, 7);

    msg[7] = body_len >> 8;
    msg[8] = body_len - (msg[7] << 8);

    if (body_len > 0) {
        strcpy(&msg[9], cmd_body);
    }
#ifdef VERBOSE
    std::cout << "sent " << cmd_id << std::endl;
#endif

    ssize_t sentlen;
    if((sentlen = send(sock, msg, msglen, 0)) < 0 ) {
        die_on_error();
    }
    return sock;
}

/*
 * broadcast
 *   send a command to all connected peers
 *   @param cmd_id   command to send (should be a constant defined here or in
 *                   messages.h
 *   @param cmd_body body of the message to send. can be NULL for no body.
 *   @param body_len length of the body. must be 0 if cmd_body is NULL
 *   @return a vector of the connection file descriptors used
 */
std::vector<int> Daemon::broadcast(const char *cmd_id, const char *cmd_body, int body_len) {
    std::vector<int> sockfds;

    Peer *next = peer_set;

    do {
        if (next->id != this->peer_id) {
            sockfds.push_back(send_command(cmd_id, cmd_body, body_len, &next->address));
        }
        next = next->next;
    } while (next != peer_set);

    return sockfds;
}

/*
 * close_all
 *   close a list of file descriptors
 *   @param fd_list the list of file descriptors to close
 */
void Daemon::close_all(std::vector<int> fd_list) {
    for(size_t i = 0; i < fd_list.size(); ++i) {
        close(fd_list.at(i));
    }
}

/*
 * wait_for_all
 *   wait for a message on each socket provided
 *   @param fd_list a list of socket file descriptors to listen on
 *   @return msgs a list of Message pointers, one from each socket.
 */
std::vector<Daemon::Message*> Daemon::wait_for_all(std::vector<int> fd_list) {
    std::vector<Daemon::Message*> msgs;
    for(size_t i = 0; i < fd_list.size(); ++i) {
       msgs.push_back(receive_message(fd_list.at(i)));
    }

    return msgs;
}

/*
 * add_content_to_map
 *   adds a piece of content to this peer's map and assigns it a new key
 *   @param content - the string of content to be added to the map
 *   @return key - the key of the newly-added content
 */

int Daemon::add_content_to_map(std::string content) {
    int key = peer_id * 1000 + key_counter;
    key_counter++;
    key_map.insert(std::pair<int, std::string>(key, content));
    me()->key_count = key_map.size();

    return key;
}

/*
 * remove_key_from_map
 * Removes a key from this peer, then checks that the network is still balanced
 * If not, this peer will take a key from the peer in the network that last received
 * a piece of content (the peer that is previous to the current clock hand).
 *
 * Note: To keep things synchronous, this peer must broadcast the UPDATE_TOTALS message
 *       on behalf of the peer that it stole a key from. The UPDATE_TOTALS message signals
 *       to the original recipient of the request that it can send an ACKNOWLEDGE back to the
 *       client, which we don't want to do until the the new key distribution has been settled
 *
 * @param key - the key to be removed
 * @return did_rebalance - true if the operation caused a rebalance, false otherwise
 */
bool Daemon::remove_key_from_map(int key) {
    bool rebalance = will_rebalance();
    key_map.erase(key);

    if (rebalance) {
        do {
            peer_set = peer_set->previous;
            broadcast_tick_back();
        } while (peer_set->key_count < balance_max() || peer_set == me());

        int sockfd = send_steal_key(peer_set);
        Message *resp = receive_message(sockfd);
        process_steal_response(resp);
        int new_size = key_map.size() - 1;
        peer_set->key_count = new_size;
        broadcast_update_totals(new_size, peer_set->id);
        delete resp;
    } else {
        broadcast_update_totals();
    }

    return rebalance;
}

/*
 * resolve_remove_protocol
 *   carries out a bit of back and forth communication with the sender of the `remove_reply` message
 *   that ensures that the table's state has fully stabilized before returning
 *   Process:
 *      - if the removal is going to result in a rebalance, wait for someone
 *        to tell us to tick the clock back
 *          - if after the clock ticks back it is now pointing to this peer, we also have to
 *            expect a STEAL_KEY message
 *      - next, wait for someone to tell us to update the totals of the peer that wound up
 *        losing a key
 *      - finally, wait for an acknowledgement that every peer has finished updating
 *
 * @param remove_reply - the message that indicated that this peer will be removing a key
 */

void Daemon::resolve_remove_protocol(Message *remove_reply) {
    std::vector<std::string> body_items = tokenize(remove_reply->body, TOKEN_SEPARATOR);
    bool did_rebalance = (bool)atoi(body_items.at(0).c_str());
    int num_ticks = atoi(body_items.at(1).c_str());

    if (did_rebalance) {
        for (int i = 0; i < num_ticks; ++i) {
            Message *tick = receive_message();
            if (!msg_command_is(tick, TICK_BACK)) {
                throw Exception(PROTOCOL_VIOLATION);
            }
            process_tick_back(tick);
            delete tick;
        }

        if (peer_set == me()) {
            Message *steal = receive_message();
            if (!msg_command_is(steal, STEAL_KEY)) {
                throw Exception(PROTOCOL_VIOLATION);
            }
            process_steal_key(steal);
            delete steal;
        }
    }

    Message *update = receive_message();
    if (!msg_command_is(update, UPDATE_TOTALS)) {
        throw Exception(PROTOCOL_VIOLATION);
    }
    process_update_totals(update);
    delete update;

    wait_for_ack(remove_reply->connection);
}

/*
 * will_rebalance
 *
 * @return true if removing a key from this peer will require a rebalance, false otherwise
 */
bool Daemon::will_rebalance() {
    Peer *next = peer_set;

    do {
        if (next->key_count > key_map.size()) {
            return true;
        }

        next = next->next;
    } while (next != peer_set);

    return false;
}

/*
 * balance_max
 *
 * @return max - the maximum number of keys a peer can have so that the network is balanced
 */
unsigned int Daemon::balance_max() {
    Peer *next = peer_set;
    int total = 0;
    int num_peers = 0;

    do {
        total += next->key_count;
        num_peers++;
        next = next->next;
    } while (next != peer_set);

    return ceil((float)total / (float)num_peers);
}

/*
 * find_peer_by_id
 *    find a peer in the network by id
 *    @param id - the integer id of the desired peer
 *    @return matching peer or null
 */
Peer *Daemon::find_peer_by_id(int id) {
    Peer *next = peer_set;
    do {
        if (next->id == id) return next;
        next = next->next;

    } while(next != peer_set);
    return NULL;
}

/*
 * find_peer_by_addr
 *   find a peer in the network by address and port
 *   @param addr dotted quad represetation of ip address
 *   @param port port in network order
 *   @return matching Peer or NULL
 */
Peer *Daemon::find_peer_by_addr(const char* addr, unsigned short sin_port) {
    Peer *next = peer_set;
    do {
        if (sin_port == next->address.sin_port &&
            strcmp(addr, inet_ntoa(next->address.sin_addr)) == 0) {

            return next;
        }

        next = next->next;

    } while (next != peer_set);

    return NULL;
}

/*
 * wait_for_ack
 *   wait for an acknowledgement, then close the socket
 *   @param sockfd - the file descriptor of the socket to wait on
 */
void Daemon::wait_for_ack(int sockfd) {
    Message *ack = receive_message(sockfd);
    if (!msg_command_is(ack, ACKNOWLEDGE)) throw Exception(PROTOCOL_VIOLATION);
    delete ack;
}

/*
 * wait_for_acks
 *   wait for each remote client to acknowledge a message, then close the socket.
 *   @param fd_list a list of socket file descriptors to listen on
 */
void Daemon::wait_for_acks(std::vector<int> fd_list) {
    std::vector<Daemon::Message*> msgs = wait_for_all(fd_list);
    for (size_t i = 0; i < msgs.size(); ++i) {
        if (strcmp(msgs[i]->command, ACKNOWLEDGE) != 0) throw Exception(PROTOCOL_VIOLATION);
        delete msgs[i];
    }
}

/*
 * connect
 *   connect this node to an existing network
 *     @param remote_ip dotted quad representation of the ip address of a peer
 *     @remote_port port of a peer, in network byte order
 */
void Daemon::connect(const char *remote_ip, unsigned short remote_port) {
    in_addr addr;
    sockaddr_in remote;
    inet_aton(remote_ip, &addr);
    remote.sin_family = AF_INET;
    remote.sin_addr = addr;
    remote.sin_port = remote_port;
    std::stringstream stream;
    stream << inet_ntoa(peer_set->address.sin_addr) << TOKEN_SEPARATOR
           << peer_set->address.sin_port;
    int sock = send_command(REQUEST_INFO, stream.str().c_str(), stream.str().length(), &remote); //can throw Exception
    Message *net_info = receive_message(sock);
    close(sock);
    if (strcmp(net_info->command, PEER_DATA) != 0) {
        throw Exception(PROTOCOL_VIOLATION);
    }
    process_peer_data(net_info);

    // rebalance the network
    int keys_to_take = balance_max();

    for (int i = 0; i < keys_to_take; ++i) {
        peer_set = peer_set->previous;
        broadcast_tick_back();

        if (peer_set == me()) {
            i--;
            continue;
        }

        int sockfd = send_steal_key(peer_set);
        Message *reply = receive_message(sockfd);
        process_steal_response(reply);
        delete reply;

        me()->key_count++;
        peer_set->key_count--;
    }

    Peer *next = peer_set;

    do {
        broadcast_update_totals(next->key_count, next->id);
        next = next->next;
    } while (next != peer_set);

    print_peers();
    delete net_info;
}

Daemon::~Daemon() {
    Peer *next = peer_set;
    do {
        Peer *tmp = next;
        next = tmp->next;
        delete tmp;
    } while(next != peer_set);
}

/*
 * me
 *   return the Peer corresponding to this node.
 */
Peer *Daemon::me() {
    Peer *next = peer_set;
    do {
        if (next->id == peer_id) return next;
        next = next->next;
    } while(next != peer_set);
    return NULL;
}

/*
 * Message: REQUEST_INFO
 * Body Format: remote_ip;remote_port
 * Actions:
 *        - assign the new peer an id (one higher than the current highest id)
 *        - broadcast the peer's data to all other peers
 *        - respond to the initial request with the peer's id and information about the network
 *        - add it to the peer list, right before the current head
 */
void Daemon::process_request_info(Message *message) {
    // broadcast new peer data
    std::vector<std::string> tokens = tokenize(message->body, TOKEN_SEPARATOR);
    in_addr addr;
    sockaddr_in peer_addr;
    inet_aton(tokens[0].c_str(), &addr);
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr = addr;
    peer_addr.sin_port = atoi(tokens[1].c_str());

    id_counter++;
    Peer *new_peer = new Peer(peer_addr, 0, id_counter);
    std::stringstream stream;
    stream << tokens[0].c_str() << TOKEN_SEPARATOR
           << tokens[1].c_str() << TOKEN_SEPARATOR
           << new_peer->id;
    wait_for_acks(broadcast(NEW_PEER, stream.str().c_str(), stream.str().length()));

    // reply to new peer with info
    stream.str(""); //reset stringstream
    stream.clear();
    stream << new_peer->id;

    Peer *next = peer_set;
    do {
        stream << TOKEN_SEPARATOR
               << next->id << TOKEN_SEPARATOR
               << next->key_count << TOKEN_SEPARATOR
               << inet_ntoa(next->address.sin_addr) << TOKEN_SEPARATOR
               << next->address.sin_port;
        next = next->next;
    } while (next != peer_set);
    send_command(PEER_DATA, stream.str().c_str(), stream.str().length(),
                 NULL, message->connection);
    close(message->connection);

    // add to peer list
    Peer *prev = peer_set->previous;
    new_peer->previous = prev;
    new_peer->next = peer_set;
    prev->next = new_peer;
    peer_set->previous = new_peer;

    print_peers();
}

/*
 * Message: NEW_PEER
 * Body format: ip_address;port;peer_id
 * Actions:
 *        - add the new peer to the peer list, right before the current head
 */
void Daemon::process_new_peer(Message *message) {
    in_addr addr;
    std::vector<std::string> tokens = tokenize(message->body, TOKEN_SEPARATOR);
    sockaddr_in peer_addr;
    inet_aton(tokens[0].c_str(), &addr);
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr = addr;
    peer_addr.sin_port = atoi(tokens[1].c_str());

    int new_id = atoi(tokens[2].c_str());
    id_counter = new_id;
    Peer *new_peer = new Peer(peer_addr, 0, new_id);
    Peer *prev = peer_set->previous;

    new_peer->previous = prev;
    new_peer->next = peer_set;
    prev->next = new_peer;
    peer_set->previous = new_peer;
    print_peers();
    send_command(ACKNOWLEDGE, NULL, 0, NULL, message->connection);
}

/*
 * Message: PEER_DATA
 * Body Format: new_peer_id;(peer_id;key_count;ip_addr;port)+
 * Actions:
 *        - assign own id and id_counter to new_peer_id
 *        - construct representation of network
 *        - add self to network right before current head
 */
void Daemon::process_peer_data(Message *message) {
    std::vector<std::string> tokens = tokenize(message->body, TOKEN_SEPARATOR);
    in_addr addr;
    peer_id = atoi(tokens[0].c_str());
    Peer *me = peer_set;
    me->id = peer_id;
    id_counter = peer_id;
    // all peers follow this one, and then the pointer is set to the one after this
    for(size_t i = 1; i < tokens.size(); i += 4) {
        inet_aton(tokens[i + 2].c_str(), &addr);
        sockaddr_in peer_addr;
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_addr = addr;
        peer_addr.sin_port = atoi(tokens[i + 3].c_str());
        Peer *new_peer = new Peer(peer_addr,
                                  atoi(tokens[i + 1].c_str()),
                                  atoi(tokens[i].c_str()));
        peer_set->next = new_peer;
        new_peer->previous = peer_set;
        peer_set = new_peer;
    }
    peer_set->next = me;
    me->previous = peer_set;

    peer_set = me->next;
}

/*
 * Message: ALL_KEYS
 * Body format: none (special)
 * Actions:
 *        - respond to message with comma-separated list of keys held by this peer, plus 0
 */
void Daemon::process_allkeys(Message *message) {
    std::stringstream stream;
    for (std::map<int, std::string>::iterator it = key_map.begin(); it != key_map.end(); ++it) {
        stream << it->first << ",";
    }
    stream << "0";
    if(send(message->connection, stream.str().c_str(), stream.str().length() + 1, 0) < 0 ) {
        die_on_error();
    }
}

/*
 * Message: PEER_REMOVAL
 * Body Format: ip_addr;port
 * Actions:
 *        - acknowledge message
 *        - remove peer from the set
 */
void Daemon::process_peer_removal(Message *message) {
    send_command(ACKNOWLEDGE, NULL, 0, NULL, message->connection);
    std::vector<std::string> tokens = tokenize(message->body, TOKEN_SEPARATOR);
    Peer *to_remove = find_peer_by_addr(tokens[0].c_str(),
                                        (unsigned short)atoi(tokens[1].c_str()));
    to_remove->previous->next = to_remove->next;
    to_remove->next->previous = to_remove->previous;
    if (peer_set == to_remove)
        peer_set = to_remove->next;
    delete to_remove;

    print_peers();
}

/*
 * Message: REMOVE_PEER
 * Format: None
 * Actions:
 *        - broadcast PEER_REMOVAL to all peers
 *        - distribute keys
 *        - send acknowledgement
 *        - terminate self
 */
void Daemon::process_client_remove_peer(Message *message) {
    Peer *self = me();
    std::stringstream stream;
    stream << inet_ntoa(self->address.sin_addr) << TOKEN_SEPARATOR
           << self->address.sin_port;
    wait_for_acks(
        broadcast(PEER_REMOVAL, stream.str().c_str(), stream.str().length())
    );
    terminated = true;

    if (peer_set == self) {
        peer_set = peer_set->next;
        // if this is the last peer don't distribute
        if (peer_set == self) {
            send_command(ACKNOWLEDGE, NULL, 0, NULL, message->connection);
            return;
        }
    }
    self->previous->next = self->next;
    self->next->previous = self->previous;
    for (std::map<int, std::string>::iterator it = key_map.begin(); it != key_map.end(); ++it) {
        std::stringstream stream;
        stream << it->first << TOKEN_SEPARATOR << it->second;
        int sock = send_command(FORCE_KEY, stream.str().c_str(), stream.str().length(), &(peer_set->address));
        wait_for_ack(sock);
        close(sock);
        peer_set = peer_set->next;
    }
    send_command(ACKNOWLEDGE, NULL, 0, NULL, message->connection);
}

/*
 * Message: ADD_CONTENT
 * Format: content;sender_id
 * Actions:
 *        - add the given content to this peer's table with a new key
 *        - return the new key back to the sender of the message
 */
void Daemon::process_add_content(Message *message) {
    std::vector<std::string> body_items = tokenize(message->body, TOKEN_SEPARATOR);
    std::string content = body_items.at(0);
    int key = add_content_to_map(body_items.at(0));
    send_key_response(message->connection, key);
    broadcast_update_totals();

    print_table();
}

/*
 * Message: UPDATE_TOTALS
 * Format: new_total;source
 * Actions:
 *        - update the peer with id source to have the total new_total
 */
void Daemon::process_update_totals(Message *message) {
    std::vector<std::string>  body_items = tokenize(message->body, TOKEN_SEPARATOR);
    int new_total = atoi(body_items.at(0).c_str());
    int source_id = atoi(body_items.at(1).c_str());

    Peer *updated = find_peer_by_id(source_id);
    updated->key_count = new_total;
    send_command(ACKNOWLEDGE, NULL, 0, NULL, message->connection);
}

/*
 * Message: TICK_FWD
 * Format: None
 * Actions:
 *        - advance this peer's clock hand forward one link
 */
void Daemon::process_tick_fwd(Message *message) {
    peer_set = peer_set->next;
    send_command(ACKNOWLEDGE, NULL, 0 , NULL, message->connection);
}

/*
 * Message: TICK_BACK
 * Format: None
 * Actions:
 *        - move this peer's clock hand one link backward
 */
void Daemon::process_tick_back(Message *message) {
    peer_set = peer_set->previous;
    send_command(ACKNOWLEDGE, NULL, 0, NULL, message->connection);
}

/*
 * Message: GET_KEY
 * Format: key
 * Actions:
 *        - check if this peer has the requested key, send a CONTENT_RESPONSE if it does
 *        - else send a NO_KEY response
 */
void Daemon::process_get_key(Message *message) {
    int lookup_key = atoi(message->body);
    std::map<int, std::string>::iterator result = key_map.find(lookup_key);

    if (result != key_map.end()) {
        send_content_response(message->connection, result->second.c_str());
    } else {
        send_no_key_response(message->connection);
    }
}

/*
 * Message: REMOVE_KEY
 * Format: key
 * Actions:
 *        - First, determine if this peer has the key. Reply with NO_KEY if it doesn't
 *        - If we do have the key, check if removing it will cause a rebalance, and
 *          respond with the answer
 *        - invoke remove_key_from_map, which will remove the desired key and
 *          perform any rebalancing required
 *        - send a final ACKNOWLEDGE to the requester, indicating that we are finished with
 *          the operation
 */
void Daemon::process_remove_key(Message *message) {
    int remove_key = atoi(message->body);
    std::map<int, std::string>::iterator result = key_map.find(remove_key);

    if (result != key_map.end()) {
        send_remove_key_response(message->connection, (int)will_rebalance());
        remove_key_from_map(remove_key);
        send_command(ACKNOWLEDGE, NULL, 0, NULL, message->connection);
    } else {
        send_no_key_response(message->connection);
    };

    print_table();
}

/*
 * Message: STEAL_KEY
 * Format: None
 * Actions:
 *        - remove an arbitrary key-value pair from this table and send it back as a response
 *
 *  Note: it is the responsibility of the requester to update the totals after adds the stolen
 *        key to its own table
 */
void Daemon::process_steal_key(Message *message) {
    std::map<int, std::string>::iterator first_pair = key_map.begin();
    int key = first_pair->first;
    std::string content = first_pair->second;
    key_map.erase(first_pair);
    send_steal_response(message->connection, key, content.c_str());

    print_table();
}

/*
 * Message: STEAL_RESPONSE
 * Format: key;content
 * Actions:
 *        - add the key-value pair that was given to us in the response to this peer's table
 */
void Daemon::process_steal_response(Message *message) {
    std::vector<std::string> body_items = tokenize(message->body, TOKEN_SEPARATOR);
    int key = atoi(body_items.at(0).c_str());
    std::string content = body_items.at(1);

    key_map.insert(std::pair<int, std::string>(key, content));
}

/*
 * Message: C_ADD_CONTENT
 * Format: content
 * Actions:
 *        - tell the peer being pointed to by the clock hand to add this content
 *        - move this peer's clock hand forward and tell everyone else to do the same
 *        - broadcast this peer's new total
 *        - respond to the sender with the new key
 */
void Daemon::process_client_add_content(Message *message) {
    char *content = message->body;
    char *reply;

    if (peer_set->id == peer_id) {
        int key = add_content_to_map(content);
        broadcast_update_totals();
        reply = int_to_msg_body(key);
        print_table();
    } else {
        int sockfd = send_add_content(peer_set, content);
        Message *resp = receive_message(sockfd);
        reply = new char[strlen(resp->body) + 1];

        strcpy(reply, resp->body);
        delete resp;

        Message *update_msg = receive_message();
        if(!msg_command_is(update_msg, UPDATE_TOTALS)){
            throw Exception(PROTOCOL_VIOLATION);
        }
        process_update_totals(update_msg);
        delete update_msg;
    }

    peer_set = peer_set->next;
    broadcast_tick_fwd();

    send_command(ACKNOWLEDGE, reply, strlen(reply) + 1, NULL, message->connection);
    delete reply;
}

/*
 * Message: C_LOOKUP_CONTENT
 * Format: key
 * Actions:
 *        - check if this peer has the content with the requested key. if it does, return it
 *        - else broadcast a GET_KEY message looking for the content, gather responses
 *        - if any of the responses is a CONTENT_RESPONSE, respond to the client with the content found
 *        - otherwise respond with a FAIL response
 */
void Daemon::process_client_lookup_content(Message *message) {
    int lookup_key = atoi(message->body);
    std::map<int, std::string>::iterator result = key_map.find(lookup_key);

    if (result != key_map.end()) {
        send_content_response(message->connection, result->second.c_str());
    } else {
        std::vector<int> sockfds = broadcast_get_key(lookup_key);
        std::vector<Message*> replies = wait_for_all(sockfds);
        bool found = false;

        for (size_t i = 0; i < replies.size(); ++i) {
            Message *reply = replies.at(i);
            if (msg_command_is(reply, CONTENT_RESPONSE)) {
                found = true;
                send_content_response(message->connection, reply->body);
            }

            delete reply;
        }

        if (!found) {
            send_fail_response(message->connection);
        }
    }
}

/*
 * Message: C_REMOVE_CONTENT
 * Format: key
 * Actions:
 *        - look for the requested content in the current peer. if found, remove it and update totals
 *          - if removed and the current peer now has too few keys, tick the clock backward and then take a
 *            key from that peer. Then wait to receive a UPDATE_TOTALS message before sending
 *            an ACKNOWLEDGE to the client
 *        - else broadcast a REMOVE_KEY message to the network, and collect responses
 *          - return a FAIL message to the client if all responses are NO_KEY
 *          - if there exists another peer with the requested key, resolve the removal protocol
 */
void Daemon::process_client_remove_content(Message *message) {
    int remove_key = atoi(message->body);
    std::map<int, std::string>::iterator result = key_map.find(remove_key);

    if (result != key_map.end()) {
        remove_key_from_map(remove_key);
    } else {
        std::vector<int> sockfds = broadcast_remove_key(remove_key);
        std::vector<Message*> replies = wait_for_all(sockfds);
        bool found = false;

        for (size_t i = 0; i < replies.size(); ++i) {
            Message *reply = replies.at(i);

            if (!msg_command_is(reply, NO_KEY)) {
                found = true;
                resolve_remove_protocol(reply);
            }

            delete reply;
        }

        if (!found) {
           send_fail_response(message->connection);
           return;
        }
    }

    send_command(ACKNOWLEDGE, NULL, 0, NULL, message->connection);
}

/**
 * Message: FORCE_KEY
 * Format: key,value
 * Actions:
 *        - add key and value to map
 */
void Daemon::process_force_key(Message *message) {
    std::vector<std::string> tokens = tokenize(message->body, TOKEN_SEPARATOR);
    key_map.insert(std::pair<int, std::string>(atoi(tokens[0].c_str()), tokens[1]));
    me()->key_count = key_map.size();

    broadcast_update_totals();
    broadcast_tick_fwd();
    send_command(ACKNOWLEDGE, NULL, 0, NULL, message->connection);

    print_table();
}

/*
 * Message Purpose
 *   broadcasts a message indicating that all peers should move their clock hands forward
 *
 * This message has no body
 */
void Daemon::broadcast_tick_fwd() {
    wait_for_all(broadcast(TICK_FWD, NULL, 0));
}

/*
 * Message Purpose
 *   broadcasts a message indicating that all peers should move their clock hands backward
 *
 * This message has no body
 */
void Daemon::broadcast_tick_back() {
    wait_for_acks(broadcast(TICK_BACK, NULL, 0));
}

/*
 * Message Purpose
 *   broadcasts a message indicating that this peer has updated the number of keys it holds
 *   will wait for acknowledgement from the network before continuing
 *
 * Message Body Format: new_total;id
 *   new_total - integer
 *      the new number of keys at this peer
 *   peer_to_update - integer
 *      the id of the peer that needs to be updated
 *
 */
void Daemon::broadcast_update_totals(int total, int id) {
    if (id == -1) {
        id = peer_id;
    }

    if (total == -1) {
        total = key_map.size();
    }

    char *new_total = int_to_msg_body(total);
    char *peer_to_update = int_to_msg_body(id);
    char *body = new char[strlen(new_total) + strlen(peer_to_update) + 2];
    strcpy(body, new_total);
    body[strlen(new_total)] = TOKEN_SEPARATOR_CHAR;
    strcpy(&body[strlen(new_total) + 1], peer_to_update);

    std::vector<int> sockfds = broadcast(UPDATE_TOTALS, body, strlen(body) + 1);
    wait_for_acks(sockfds);

    delete[] new_total;
    delete[] peer_to_update;
    delete[] body;
}

/*
 * Message Purpose
 *   broadcasts a request to remove a particular key from the DHT
 *
 * Message Body Format: key
 *   key - integer
 *      the key to be removed
 */
std::vector<int> Daemon::broadcast_remove_key(int key) {
    char *body = int_to_msg_body(key);
    std::vector<int> sockfds = broadcast(REMOVE_KEY, body, strlen(body) + 1);
    delete[] body;

    return sockfds;
}

/*
 * Message Purpose
 *   broadcasts a request to retrieve the value of a particular key from the DHT
 *
 * Message Body Format: key;source
 *   key - integer
 *      the key of the content we want to return
 *   source - integer
 *      the id of the peer that is requesting the content
 */

std::vector<int> Daemon::broadcast_get_key(int key) {
    char *key_str = int_to_msg_body(key);
    char *source = int_to_msg_body(peer_id);
    char *body = new char[strlen(key_str) + strlen(source) + 2];

    strcpy(body, key_str);
    body[strlen(key_str)] = TOKEN_SEPARATOR_CHAR;
    strcpy(&body[strlen(key_str) + 1], source);

    std::vector<int> sockfds = broadcast(GET_KEY, body, strlen(body) + 1);

    delete[] key_str;
    delete[] source;
    delete[] body;

    return sockfds;
}

/*
 * Message Purpose
 *   sends a string to the target peer (usually as a response to a request)
 *
 * Message Body Format: content
 *   content - char*
 *      the string we want to send
 */

void Daemon::send_content_response(int sockfd, const char* content) {
    char *body = new char[strlen(content) + 1];
    strcpy(body, content);

    send_command(CONTENT_RESPONSE, body, strlen(body) + 1, NULL, sockfd);

    delete[] body;
}


/*
 * Message Purpose
 *   sends a string to the target peer with the intent to add it as a new key
 *
 * Message Body Format: content;source
 *   content - char*
 *      the string we want to send
 *   source - int
 *      the id of the peer that sent the message
 */

int Daemon::send_add_content(Peer *dest, const char* content) {
    char *source = int_to_msg_body(peer_id);
    char *body = new char[strlen(content) + strlen(source) + 2];

    strcpy(body, content);
    body[strlen(content)] = TOKEN_SEPARATOR_CHAR;
    strcpy(&body[strlen(content) + 1], source);

    int sockfd = send_command(ADD_CONTENT, body, strlen(body) + 1, &(dest->address));

    delete[] source;
    delete[] body;

    return sockfd;
}

/*
 * Message Purpose
 *   response to an add_content request that sends the key back to the requester
 *
 * Message Body Format: key
 *   key - int
 *      the key of the newly-added content
 */

void Daemon::send_key_response(int sockfd, int key) {
    char *body = int_to_msg_body(key);
    send_command(KEY_RESPONSE, body, strlen(body) + 1, NULL, sockfd);

    delete[] body;
}

/*
 * Message Purpose
 *   tells a peer that a requested key was not found (in reply to a get key request)
 *
 * This message has no body
 */
void Daemon::send_no_key_response(int sockfd) {
    send_command(NO_KEY, NULL, 0, NULL, sockfd);
}

/*
 * Message Purpose
 *   tells the client that a request has failed
 *
 * This message has no body
 */
void Daemon::send_fail_response(int sockfd) {
    send_command(FAIL, NULL, 0, NULL, sockfd);
}

/*
 * Message Purpose
 *   acknowledges that a key has been found and will be removed
 *
 * Message Body: will_rebalance;num_ticks
 *  will_rebalance - int
 *      Will be 1 if a rebalance is required to remove this key, 0 otherwise
 *  num_ticks - int
        Only nonzero if will_rebalance is 1. indicates the number of times the clock
        will tick back
 */
void Daemon::send_remove_key_response(int sockfd, int will_rebalance) {
    int num_ticks = 0;

    if (will_rebalance) {
        Peer *prev = peer_set;

        do {
           prev = prev->previous;
           num_ticks++;
        } while (prev->key_count < balance_max() || prev == me());
    }

    char *num_ticks_str = int_to_msg_body(num_ticks);
    char *rebalance_str = int_to_msg_body(will_rebalance);
    char *body = new char[strlen(rebalance_str) + strlen(num_ticks_str) + 2];
    strcpy(body, rebalance_str);
    body[strlen(rebalance_str)] = TOKEN_SEPARATOR_CHAR;
    strcpy(&body[strlen(rebalance_str) + 1], num_ticks_str);

    send_command(REMOVE_RESPONSE, body, strlen(body) + 1, NULL, sockfd);

    delete[] num_ticks_str;
    delete[] rebalance_str;
    delete[] body;
}

/*
 * Message Purpose
 *   asks (very politely) for the peer dest to give this peer a key
 *
 * This message has no body
 */
int Daemon::send_steal_key(Peer *dest) {
    return send_command(STEAL_KEY, NULL, 0, &(dest->address));
}

/*
 * Message Purpose
 *   responds to a steal request with a key and the content associated with it
 * Message Body: key;content
 *   key - int
 *      The key that is being stolen
 *   content - char*
 *      The content associated with the stolen key
 */
void Daemon::send_steal_response(int sockfd, int key, const char *content) {
    char *key_str = int_to_msg_body(key);
    char *body = new char[strlen(key_str) + strlen(content) + 2];

    strcpy(body, key_str);
    body[strlen(key_str)] = TOKEN_SEPARATOR_CHAR;
    strcpy(&body[strlen(key_str) + 1], content);

    send_command(STEAL_RESPONSE, body, strlen(body) + 1, NULL, sockfd);

    delete[] key_str;
    delete[] body;
}

