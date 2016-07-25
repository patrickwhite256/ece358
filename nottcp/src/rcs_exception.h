#ifndef RCS_EXCEPTION
#define RCS_EXCEPTION

struct RCSException {
    int err_code;

    RCSException(int err_code) : err_code(err_code) {}
};

// Error types
#define UNDEFINED_SOCKFD 1

#endif
