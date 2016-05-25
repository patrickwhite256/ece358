class Daemon {
    void send_command(char* cmd_id, char* cmd_body, int body_len, sockaddr_in dest);
  public:
    void loop(int sockfd);
};

void die_on_error();
