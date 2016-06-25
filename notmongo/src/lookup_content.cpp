#include <algorithm>
#include <iostream>
#include <cstring>
#include <unistd.h>

#include "basic_exception.h"
#include "util.h"
#include "client.h"
#include "messages.h"

using namespace std;

int main(int argc, char **argv) {
    if (argc != 4) {
        cerr << "Usage: lookupcontent ip-address port key" << endl;
        exit(-1);
    }

    int content_length = strlen(argv[3]);

    try {
        Client client(argv[1], atoi(argv[2]));
        client.send_command(C_LOOKUP_CONTENT, argv[3], content_length);
        char *resp = client.receive();
        cout << resp << endl;
        delete[] resp;
    } catch (Exception ex) {
        if (ex.err_code() == BAD_ADDRESS) {
            cerr << "Error: no such peer" << std::endl;
        } else {
            cerr << "Error: no such content" << endl;
        }
        exit(-1);
    }
}
