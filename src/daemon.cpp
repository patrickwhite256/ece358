#include <cstring>
#include <algorithm>
#include <arpa/inet.h>
#include <iostream>
#include <netinet/tcp.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>

#include "util.h"
#include "daemon.h"
#include "peer.h"
#include "basic_exception.h"

const int INITIAL_BUFFER_LEN = 300;
// messages that can be heard while listening
const char *ALL_KEYS         = "allkeys";
const char *ADD_KEY          = "addakey";
const char *REQUEST_INFO     = "reqinfo";
const char *NEW_PEER         = "newpeer";

// messages that are part of protocols
const char *PEER_DATA        = "peerdta";

Daemon::Daemon(int sockfd) {
    this->sockfd = sockfd;
    this->peer_id = 0; // assume we are the first node until told otherwise
    sockaddr_in dummy;
    peer_set = new Peer(dummy, 0, 0);
    peer_set->next = peer_set;
    peer_set->previous = peer_set;
}

void Daemon::loop() {
    if(listen(sockfd, 0) < 0) {
        die_on_error();
    }

    while(true) {
        Daemon::Message *message = receive_message();
        if(strcmp(message->command, ALL_KEYS) == 0) {
        } else if (strcmp(message->command, ADD_KEY) == 0) {
        } else if (strcmp(message->command, REQUEST_INFO) == 0) {
            process_request_info(message);
        } else if (strcmp(message->command, NEW_PEER) == 0) {
            process_new_peer(message);
        }
        delete message;
    }
}

Daemon::Message *Daemon::receive_message() {
    unsigned char initial_buffer[INITIAL_BUFFER_LEN];
    sockaddr_in client;
    socklen_t alen = sizeof(sockaddr_in);
    int connectedsock = accept(sockfd, (sockaddr *)&client, &alen);
    if(connectedsock < 0) {
        die_on_error();
    }
    std::cout << "connected" << std::endl;
    ssize_t recv_len = recv(connectedsock, initial_buffer,
                            INITIAL_BUFFER_LEN, 0);
    if(recv_len < 0) {
        die_on_error();
    }
    std::cout << "received " << recv_len << std::endl;

    char *cmd = new char[8];
    memcpy(cmd, initial_buffer, 7);
    cmd[7] = '\0';
    std::cout << cmd << std::endl;

    if(strcmp(cmd, ALL_KEYS) == 0) {
        char *dummy = new char[1];
        return new Daemon::Message(cmd, 0, dummy, client);
    } else {
        int msg_size = (initial_buffer[7] << 8) + initial_buffer[8];
        char *contents = new char[msg_size + 1];
        int msg_remaining = msg_size;
        int amt_from_first_packet = std::min(msg_remaining, INITIAL_BUFFER_LEN - 9);
        memcpy(contents, &initial_buffer[9], amt_from_first_packet);
        msg_remaining -= amt_from_first_packet;
        while(msg_remaining) {
            recv_len = recv(connectedsock, &contents[msg_size - msg_remaining],
                            msg_remaining, 0);
            msg_remaining -= recv_len;
        }
        contents[msg_size] = '\0';

        return new Daemon::Message(cmd, msg_size, contents, client);
    }
}

void Daemon::send_command(const char *cmd_id, const char *cmd_body, int body_len, sockaddr_in *dest) {
    int sock = -1;
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

    size_t msglen = body_len + 9;
    char msg[msglen];
    memcpy(msg, cmd_id, 7);

    msg[7] = body_len >> 8;
    msg[8] = body_len - (msg[7] << 8);

    if (body_len > 0) {
        strcpy(&msg[9], cmd_body);
    }

    ssize_t sentlen;
    if((sentlen = send(sock, msg, msglen, 0)) < 0 ) {
        die_on_error();
    }

    if(shutdown(sock, SHUT_RDWR) < 0) {
        die_on_error();
    }
}

void Daemon::broadcast(const char *cmd_id, const char *cmd_body, int body_len) {
    Peer *next = peer_set;

    do {
        if (next->id != this->peer_id) {
            send_command(cmd_id, cmd_body, body_len, &next->address);
        }
        next = next->next;
    } while (next != peer_set);
}

void Daemon::connect(const char *remote_ip, unsigned short remote_port) {
    in_addr addr;
    sockaddr_in remote;
    inet_aton(remote_ip, &addr);
    remote.sin_family = AF_INET;
    remote.sin_addr = addr;
    remote.sin_port = remote_port;
    std::cout << inet_ntoa(remote.sin_addr) << std::endl;
    std::cout << remote.sin_port << std::endl;
    send_command(REQUEST_INFO, (char *)"", 0, &remote); //can throw Exception
    Message *net_info = receive_message();
    if (strcmp(net_info->command, PEER_DATA) != 0) {
        throw Exception(PROTOCOL_VIOLATION);
    }
    process_request_info(net_info);
    // TODO: get keys from other peers
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

// a REQUEST_INFO message means a new peer is joining the network.
// what the node must do:
// assign the new peer an id (one higher than the current highest id)
// broadcast the peer's data to the other peers
// respond to the initial request with the peer's new id and data about all the other peers
// add it to the peer list, right before the current peer
void Daemon::process_request_info(Message *message) {
    // determine max id
    int max_id = 0;
    Peer *next = peer_set;
    do {
        if (next->id > max_id) {
            max_id = next->id;
        }
        next = next->next;
    } while (next != peer_set);

    // broadcast new peer data
    Peer *new_peer = new Peer(message->client, 0, max_id + 1);
    std::stringstream stream;
    stream << inet_ntoa(message->client.sin_addr) << ";"
           << message->client.sin_port << ";"
           << new_peer->id;
    broadcast(NEW_PEER, stream.str().c_str(), stream.str().length());

    // reply to new peer with info
    stream.str(""); //reset stringstream
    stream.clear();
    stream << new_peer->id;
    // currently next = peer_set
    do {
        stream << ";"
               << next->id << ";"
               << next->key_count << ";"
               << inet_ntoa(next->address.sin_addr) << ";"
               << next->address.sin_port;
        next = next->next;
    } while (next != peer_set);
    send_command(PEER_DATA, stream.str().c_str(), stream.str().length(),
                 &message->client);

    // add to peer list
    Peer *prev = peer_set->previous;
    new_peer->previous = prev;
    new_peer->next = peer_set;
    prev->next = peer_set;
    peer_set->previous = peer_set;
}

// NEW_PEER message format:
// ip_address;port;peer_id
// a NEW_PEER message means that a new peer is joining the network.
// it has already been received by another node.
// what the node must do:
// add the new peer to the peer list, right before the current peer
void Daemon::process_new_peer(Message *message) {
    in_addr addr;
    std::vector<std::string> tokens = tokenize(message->body, ";");
    sockaddr_in peer_addr;
    inet_aton(tokens[0].c_str(), &addr);
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr = addr;
    peer_addr.sin_port = atoi(tokens[1].c_str());
    Peer *new_peer = new Peer(peer_addr, 0, atoi(tokens[2].c_str()));
    Peer *prev = peer_set->previous;

    new_peer->previous = prev;
    new_peer->next = peer_set;
    prev->next = peer_set;
    peer_set->previous = peer_set;
}

// PEER_DATA message format:
// new_peer_id;<peerdata>+
//   peerdata: peer_id;key_count;ip_addr;port
void Daemon::process_peer_data(Message *message) {
    std::vector<std::string> tokens = tokenize(message->body, ";");
    in_addr addr;
    peer_id = atoi(tokens[0].c_str());
    Peer *me = peer_set;
    me->id = peer_id;
    // all peers follow this one, and then the pointer is set to the one after this
    for(size_t i = 1; i < tokens.size(); i += 4) {
        inet_aton(tokens[i + 2].c_str(), &addr);
        sockaddr_in peer_addr;
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_addr = addr;
        peer_addr.sin_port = atoi(tokens[i + 3].c_str());
        Peer *new_peer = new Peer(peer_addr,
                                  atoi(tokens[i + 9].c_str()),
                                  atoi(tokens[i].c_str()));
        peer_set->next = new_peer;
        new_peer->previous = peer_set;
        peer_set = new_peer;
    }
    peer_set->next = me;
    me->previous = peer_set;

    peer_set = me->next;
}
