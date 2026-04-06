  #ifndef SERVER_H
    #define SERVER_H
    #define BACKLOG 20          
    int server_init(const char *port);
    int server_run(int sockfd);
  #endif