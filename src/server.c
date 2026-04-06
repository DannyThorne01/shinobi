  #include <stdio.h>
  #include <stdlib.h>        // exit()
  #include <string.h>        // memset()
  #include <unistd.h>        // close(), fork()
  #include <signal.h>        // sigaction, SIGCHLD
  #include <sys/wait.h>      // wait()
  #include <sys/socket.h>    // accept, recv, send
  #include <netdb.h>         // getaddrinfo
  #include <arpa/inet.h>     // inet_ntop
  #include "utils.h"         // LOG_DEBUG, LOG_INFO, addr_to_str
  #include "http.h"          // HttpRequest, HttpParseResult, http_parse_request
  #include "handler.h"       // http_handler
  #include "server.h"        // own declarations + BACKLOG
/*
  struct addrinfo {
      int               ai_flags;         // Input flags (AI_PASSIVE, AI_CANONNAME, etc.)
      int               ai_family;        // Protocol family (AF_INET, AF_INET6, AF_UNSPEC)
      int               ai_socktype;      // Socket type (SOCK_STREAM, SOCK_DGRAM)
      int               ai_protocol;      // Protocol (IPPROTO_TCP, IPPROTO_UDP, or 0)
      size_t            ai_addrlen;       // Length of ai_addr in bytes
      char             *ai_canonname;     // Canonical name of the host
      struct sockaddr  *ai_addr;          // Pointer to a socket address structure
      struct addrinfo  *ai_next;          // Pointer to the next structure in a linked list
  };

  struct sockaddr_in {
      short int           sin_family;     // Address family
      unsigned short int  sin_port;       // Port number
      struct in_addr      sin_addr;       // Internet address
      unsigned char       sin_zero[8];    // Same size as struct sockaddr
  };
*/

void sigchld_handler(int s)
{
  while(wait(NULL) > 0);
}

int server_init(const char *port){
  int sockfd, status;
  int yes = 1;
  struct addrinfo hints, *servinfo, *p;
  char ipstr[INET6_ADDRSTRLEN];
  struct sigaction sa;

  // set the hints to zero
  //void *memset(void *ptr, int value, size_t num);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; 

  //    int getaddrinfo(
  //          const char *node,     // e.g., "www.example.com" or IP address
  //          const char *service,  // e.g., "http" or port number "80"
  //          const struct addrinfo *hints, // criteria for selection
  //          struct addrinfo **res); 
  if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0){
    LOG_DEBUG("getaddrinfo: %s", gai_strerror(status)); 
    return 1;
  }

  if(servinfo==NULL){
    LOG_DEBUG("Server: failed to bind");
    exit(1);
  }

  // loop through them and try to find one to bind to
  for(p=servinfo; p!=NULL; p=p->ai_next){

    //int socket(int domain, int type, int protocol);
    if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1){
      LOG_DEBUG("server: socket");
      continue;
    }
    // so we can reuse the IP
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
      LOG_DEBUG("setsockopt : error");
      exit(1);
    }
    // int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
    // bind it to the port we passed in to getaddrinfo():
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
      close(sockfd);
      LOG_DEBUG("server: bind");
      continue;
    }
    addr_to_str(p->ai_addr, ipstr);
    break;
  }

  freeaddrinfo(servinfo);
  
  if (listen(sockfd, BACKLOG) == -1) {
    LOG_DEBUG("listen");
    exit(1);
  }

  sa.sa_handler= sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    LOG_DEBUG("sigaction");
    exit(1);
  }

  LOG_INFO("Listening on %s:%s\n", ipstr, port);
  fflush(NULL);
  return sockfd;
}

int server_run(int sockfd){
  int new_fd;
  char peer_name[INET6_ADDRSTRLEN];
  struct sockaddr_storage their_addr;

  while(1){
     //int accept(int sockfd, void *addr, int *addrlen);
    socklen_t addr_size = sizeof(their_addr);
    if((new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &addr_size))==-1){
      LOG_DEBUG("Accept Error");
      continue;
    }
    addr_to_str((struct sockaddr *)&their_addr, peer_name);
    LOG_INFO("Server: Got Connection From %s\n", peer_name);
    fflush(NULL);

    if(!fork()){
      close(sockfd);
      // stores the meta data we get from response
      char buf[4096];
      memset(buf, 0, sizeof(buf));

      //recv(fd, buf, len, flags)
      int bytes_received = recv(new_fd, buf, sizeof(buf) - 1, 0);
      if (bytes_received == -1) {
          close(new_fd);
          LOG_DEBUG("RECV Error");
          exit(1);
      }else{
        LOG_INFO("--- Request: ---\n%s\n--- End ---\n", buf);
        HttpRequest http_request = {0};
        HttpParseResult parse_result = http_parse_request(buf, &http_request);
        if (parse_result == HTTP_OK) {
          http_handler(&http_request, new_fd);
        } else {
          send_404(new_fd);  
        }
     
        if (http_request.user_agent) free(http_request.user_agent);
        close(new_fd);
        exit(0);
      }
    }
    close(new_fd); // parent closes its copy
  }
  return 0;
}