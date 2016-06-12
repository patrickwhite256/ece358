#ifndef MESSAGES_H
#define MESSAGES_H

// messages sent from clients
extern const char *ALL_KEYS;
extern const char *REMOVE_PEER;
extern const char *C_ADD_CONTENT;
extern const char *C_LOOKUP_CONTENT;

// message sent to client on success
extern const char *ACKNOWLEDGE;

// message sent to client on failure
extern const char *FAIL;

#endif
