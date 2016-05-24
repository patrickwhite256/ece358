class PeerDaemon {
    void send_command(void);
  public:
    void loop(int sockfd);
};

void die_on_error();
