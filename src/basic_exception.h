#include <exception>
#include <string>

#define BAD_ADDRESS 1

class Exception: public std::exception {
    int _err_code;

  public:
    explicit Exception(int err_code) : _err_code(err_code) {}
    virtual ~Exception() throw() {}

    virtual const char* what() const throw () {
        std::string msg;
        switch (_err_code) {
            case BAD_ADDRESS:
                msg = "This is a bad address";
                break;
            default:
                msg = "Some unclassified error happened";
        }

        return msg.c_str();
    }
};
