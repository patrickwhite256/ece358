#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <sstream>

#include "basic_exception.h"
#include "util.h"
#include "client.h"
#include "messages.h"

using namespace std;

int main(int argc, char **argv) {
    if (argc != 3) {
        cerr << "Usage: removepeer [ip-address] [port]" << endl;
        exit(-1);
    }
    try {
        Client client(argv[1], atoi(argv[2]));
        client.send_command(REMOVE_PEER, "", 0);
        client.wait_for_ack();
    } catch (Exception ex) {
        cerr << "Error: no such peer" << endl;
        exit(-1);
    }
}
