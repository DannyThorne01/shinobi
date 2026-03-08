#include "stdio.h"  // printf, fprintf
#include <stdlib.h>      // exit()
#include "string.h"      // memset()
#include <errno.h>
#include <arpa/inet.h>   // inet_ntop() - converts binary IP to string
#include <netinet/in.h>  // sockaddr_in structures
#include <netdb.h>       // getaddrinfo(), addrinfo

#include <sys/wait.h>    // wait()
#include <sys/signal.h>
#include <sys/socket.h>  // socket types/constants  
#include <sys/types.h>   // data types (size_t)

#include <unistd.h>      // fork()
#include <signal.h>

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
#define MY_PORT "3490"
#define QUEUE_SIZE 20

void addr_to_str(struct sockaddr *addr, char *res){
  void* res_addr;
  char *ipver;
  if(addr->sa_family == AF_INET){
      ipver = "IPv4";
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr; 
      res_addr = &ipv4->sin_addr;
      uint8_t *bytes = (uint8_t *)res_addr;
      printf("IPv4 RAW BYTES: %02x %02x %02x %02x\n", 
        bytes[0], bytes[1], bytes[2], bytes[3]);
  }else{
      ipver = "IPv6";
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) addr;
      res_addr = &ipv6->sin6_addr;
      uint8_t *bytes = (uint8_t *)res_addr;
      printf("IPv6 RAW BYTES: %02x %02x %02x %02x %02x %02x %02x %02x"
             " %02x %02x %02x %02x %02x %02x %02x %02x\n",
        bytes[0],  bytes[1],  bytes[2],  bytes[3],
        bytes[4],  bytes[5],  bytes[6],  bytes[7],
        bytes[8],  bytes[9],  bytes[10], bytes[11],
        bytes[12], bytes[13], bytes[14], bytes[15]);
    }
  inet_ntop(addr->sa_family, res_addr, res, INET6_ADDRSTRLEN);
  printf("%s: %s\n", ipver, res);
}

void sigchld_handler(int s)
{
  while(wait(NULL) > 0);
}

int main(){
  int status;
  struct addrinfo hints = {}; //search criteria
  struct addrinfo *servinfo, *winner; // Results lists + best match
  char ipstr[INET6_ADDRSTRLEN];
  socklen_t addr_size;
  struct sigaction sa;
  int yes = 1;
  int new_fd;
  struct sockaddr_storage their_addr;
 
  memset(&hints, 0, sizeof(hints)); // Fill form with zeros
  // fill specifc boxes
  hints.ai_family = 0; // "Try everything both IPv4 and IPv6"
  hints.ai_socktype = SOCK_STREAM; // TCP please
  hints.ai_flags = 0; // I'm a server

  if((status = getaddrinfo(NULL, MY_PORT, &hints, &servinfo)) != 0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 1;
  }
  if(servinfo==NULL){
    printf("getaddrinfo: could not find result");
    return 1;
  }

  winner = servinfo;
  // make a socket and bind 
  addr_to_str(servinfo->ai_addr, ipstr);
  //int socket(int domain, int type, int protocol);
  int sockfd = socket(winner->ai_family, winner->ai_socktype, winner->ai_protocol);

  // so we can reuse the IP
  if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
    printf("setsockopt : error");
    return 1;
  }
  // int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
  if (bind(sockfd, winner->ai_addr, winner->ai_addrlen) == -1){
    printf("bind: error");
    return 1;
  }
  freeaddrinfo(servinfo);

  if(listen(sockfd, QUEUE_SIZE) == -1){
    printf("listen: error");
    return 1;
  }
  printf("listening on %s:%s\n", ipstr, MY_PORT);
  fflush(NULL);

  sa.sa_handler= sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
  while(1){
    addr_size=  sizeof(struct sockaddr_storage);

    //int accept(int sockfd, void *addr, int *addrlen);
    if((new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &addr_size))==-1){
      perror("accept");
      continue;
    }
    char peer_name[INET6_ADDRSTRLEN];
    addr_to_str((struct sockaddr *)&their_addr, peer_name);
    printf("server: got connection from %s\n", peer_name);
    fflush(NULL);

    if (!fork()){ // this is the child process
      close(sockfd); // child doesn;t need a listener 
      char buf[4096];
      memset(buf, 0, sizeof(buf));

      int bytes_received = recv(new_fd, buf, sizeof(buf) - 1, 0);
      if (bytes_received == -1) {
          perror("recv");
      } else {
          // char request[10], path[256], version[16], host[50];
          // sscanf(buf, "%9s %255s %15s %*s %49s", request, path, version, host);
          // printf("Request: %s\n", request);
          // printf("Path: %s\n", path);
          // printf("Version: %s\n", version);
          // printf("Host: %s\n", host);
          printf("--- request ---\n%s\n--- end ---\n", buf);
          char method[10], uri[256], host[50];
          char* useragent = NULL;
          char* saveptr; 
          char* res; 
          char* token = strtok_r(buf, "\r\n", &saveptr);
          // Extracting the Method and the URI
          char* tok1 = strtok_r(token, " ", &res);
          strncpy(method, tok1, sizeof(method));
          char* tok2 = strtok_r(NULL, " ", &res);
          strncpy(uri, tok2, sizeof(uri));
    
          fflush(NULL);

          token = strtok_r(NULL, "\r\n", &saveptr);
          while(token!=NULL){
            char* tok1 = strtok_r(token, ":", &res);

            if(strncmp(tok1, "Host", 4) == 0){
              strncpy(host, res, sizeof(host));
            }
            else if(strncmp(tok1, "User-Agent", 10) == 0){
              useragent = calloc(strlen(res) + 1, sizeof(char));
              strncpy(useragent, res, strlen(res) + 1);
            }
            token = strtok_r(NULL, "\r\n", &saveptr);
          }
          printf("Method:%s,\nURI:%s,\nHOST:%s,\nU-Agent:%s\n", method, uri, host, useragent);
          // getting the file based on the URI
          char* filename;
          if(strncmp(uri, "/", 1)== 0){
            filename = "index.html";
          }
          char temp[300];
          
          snprintf(temp, sizeof(temp), "html%s%s", uri, filename);
          FILE* fptr;
          printf("%s", temp);
          fptr = fopen(temp,"r");
          if (fptr == NULL) {
              printf("The file is not opened.");
              return 1;
          }
          // getitng the size of the file
          fseek(fptr, 0, SEEK_END);
          long file_size = ftell(fptr);
          fseek(fptr, 0, SEEK_SET); // Reset to beginning
          char* page_buf = calloc(file_size+1, sizeof(char));
          if(page_buf == NULL){
            printf("Could Allocate Memory to read from file");
            fclose(fptr);
            return 1;
          }
          fread(page_buf, sizeof(char), file_size, fptr);
          page_buf[file_size] = 0;
          fclose(fptr);

          // create the response
          char* response = calloc(file_size+300, sizeof(char));
          snprintf(response, file_size+300,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            file_size, page_buf);

          // send the response
          if (send(new_fd, response, strlen(response), 0) == -1){
            perror("send");
          }
          
          fflush(NULL);
      }
      // const char *response =
      //   "HTTP/1.1 200 OK\r\n"
      //   "Content-Type: text/plain\r\n"
      //   "Content-Length: 13\r\n"
      //   "Connection: close\r\n"
      //   "\r\n"
      //   "Hello World!\n";
      // if (send(new_fd, response, strlen(response), 0) == -1){
      //   perror("send");
      // }
      close(new_fd);
      exit(0);
    }
    close(new_fd); //parent doesn't need this anymore
  }

return 0;

}
