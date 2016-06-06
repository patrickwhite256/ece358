#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <string>

const int INITIAL_BUFFER_LEN = 300;

void die_on_error();
std::vector<std::string> tokenize(const char *str, const char *delimiter);
char *int_to_msg_body(int i); // it is the responsibility of the caller to deallocate the char* returned by this

#endif
