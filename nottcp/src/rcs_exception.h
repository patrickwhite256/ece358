#ifndef RCS_EXCEPTION
#define RCS_EXCEPTION

struct RCSException {
    int err_code;

    RCSException(int err_code) : err_code(err_code) {}
};

// Error types
#define RCS_ERROR_UNDEFINED_SOCKFD 0
#define RCS_ERROR_UNEXPECTED_MSG   1
#define RCS_ERROR_TIMEOUT          2
#define RCS_ERROR_CORRUPT          3
#define RCS_ERROR_RESEND           4

#endif
