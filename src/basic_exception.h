#ifndef BASIC_EXCEPTION_H
#define BASIC_EXCEPTION_H

#include <exception>
#include <string>

#define BAD_ADDRESS             1
#define PROTOCOL_VIOLATION      2
#define FAILED_REQUEST          3

class Exception: public std::exception {
    int _err_code;

  public:
    explicit Exception(int err_code = 0) : _err_code(err_code) {}
    virtual ~Exception() throw() {}

    virtual const char* what() const throw () {
        std::string msg;
        switch (_err_code) {
            case BAD_ADDRESS:
                msg = "This is a bad address";
                break;
            case PROTOCOL_VIOLATION:
                msg = "Violation of protocol";
                break;
            default:
                msg = "Some unclassified error happened";
        }

        return msg.c_str();
    }

    int err_code() { return _err_code; }
};

#endif
