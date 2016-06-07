#include <algorithm>
#include <iostream>
#include <cstring>
#include <unistd.h>

#include "basic_exception.h"
#include "util.h"
#include "client.h"
#include "messages.h"

using namespace std;

int main(int argc, char**argv) {
    if (argc != 4) {
        cerr << "Usage: addcontent ip-address port \"content\"" << endl;
        exit(-1);
    }

    int content_len = strlen(argv[3] - 2);

    char *content = new char[content_len + 1];
    memcpy(content, &argv[3][1], content_len);
    content[content_len] = '\0';

    try {
        Client client(argv[1], atoi(argv[2]));
        client.send_command(C_ADD_CONTENT, content, content_len);
        cout << client.receive() << endl;
    } catch (Exception ex) {
        cerr << "Error: no peers in network" << endl;
    }
}
