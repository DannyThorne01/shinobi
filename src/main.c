#include "stdio.h"  // printf, fprintf
#include <arpa/inet.h>   // inet_ntop() - converts binary IP to string
#include <netinet/in.h>  // sockaddr_in structures
#include <sys/socket.h>  // socket types/constants  
#include <sys/types.h>   // data types (size_t)
#include <netdb.h>       // getaddrinfo(), addrinfo
#include "string.h"      // memset()


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

int main(){
  int status;
  char* site = "laminar-interview.vercel.app";
  struct addrinfo hints = {}; //search criteria
  struct addrinfo *servinfo, *winner; // Results lists + best match
  char ipstr[INET6_ADDRSTRLEN];// Buffer for IP string (46 chars max )
  // configure DNS Lookup (hints)
  memset(&hints, 0, sizeof(hints)); // Fill form with zeros
  // fill specifc boxes
  hints.ai_family = 0; // "Try everything both IPv4 and IPv6"
  hints.ai_socktype = 1; // TCP please
  hints.ai_flags = 0; // I'm a server

  if((status == getaddrinfo(site, "80", &hints, &servinfo)) != 0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 1;
  }

  for (winner = servinfo; winner !=NULL; winner = winner->ai_next){
    void *addr;
    char *ipver;
    if(winner->ai_family == AF_INET){
      ipver = "IPv4";
      struct sockaddr_in *ipv4 = (struct sockaddr_in *) winner->ai_addr; 
      addr = &ipv4->sin_addr;
      uint8_t* bytes = (uint8_t*)addr;  // Cast to bytes
      printf("IPv4 RAW BYTES: %02x %02x %02x %02x\n", 
         bytes[0], bytes[1], bytes[2], bytes[3]);
    }else{
      ipver = "IPv6";
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) winner->ai_addr;
      addr = &ipv6->sin6_addr;
      uint8_t* bytes = (uint8_t*)addr;  // Cast to bytes
      printf("IPv4 RAW BYTES: %02x %02x %02x %02x\n", 
         bytes[0], bytes[1], bytes[2], bytes[3]);
    }
    inet_ntop(winner->ai_family, addr, ipstr, sizeof ipstr);
    printf("%s: %s\n", ipver, ipstr);
  }
  freeaddrinfo(servinfo);
  return 0;
}
