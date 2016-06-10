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

    int content_len = strlen(argv[3]);

    try {
        Client client(argv[1], atoi(argv[2]));
        client.send_command(C_ADD_CONTENT, argv[3], content_len);
        char *resp = client.receive();
        cout << resp << endl;
        delete[] resp;
    } catch (Exception ex) {
        cerr << "Error: no such peer" << endl;
        exit(-1);
    }
}
