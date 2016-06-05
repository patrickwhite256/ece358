#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>

#include <iostream>

#include "util.h"

void die_on_error() {
    std::perror("error:");
    exit(-1);
}

std::vector<std::string> tokenize(const char *c_str, const char *delimiter) {
    size_t delim_len = strlen(delimiter);
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string str(c_str);
    while(true) {
        size_t next = str.find(delimiter, pos);
        if (next == std::string::npos) {
            tokens.push_back(str.substr(pos));
            break;
        } else {
            tokens.push_back(str.substr(pos, next - pos));
        }
        pos = next + delim_len;
    }
    return tokens;
}
