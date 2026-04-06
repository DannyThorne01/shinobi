#include "utils.h"
#include <stdint.h>
#include <arpa/inet.h>   // inet_ntop
#include <sys/socket.h>  // struct sockaddr, AF_INET, AF_INET6
#include <stdint.h>      // uint8_t (if you keep it)
#include <netinet/in.h>  // sockaddr_in, sockaddr_in6

//  2. Returning a pointer to a stack-allocated local variable                                                      
//   ip_addr is declared on the stack inside the function — returning a pointer to it is undefined behavior (the     
//   memory is gone after the function returns). Either pass a buffer in as a parameter, or malloc it. 

LogLevel g_log_level = LOG_LEVEL_INFO;

void addr_to_str(struct sockaddr *addr, char* ip_addr ){
  void *raw;
  if(addr->sa_family ==  AF_INET){
    struct sockaddr_in *addr_ipv4 = (struct sockaddr_in*) addr;
    raw = &addr_ipv4->sin_addr;
    uint8_t *b = (uint8_t *)raw;
    LOG_DEBUG("IPv4 raw: %02x %02x %02x %02x", b[0], b[1], b[2], b[3]);
  }
  
  else if (addr->sa_family ==  AF_INET6) {
    struct sockaddr_in6 *addr_ipv6 = (struct sockaddr_in6*) addr;
    raw = &addr_ipv6->sin6_addr;
    uint8_t *b = (uint8_t *)raw;
    LOG_DEBUG("IPv6 raw: %02x %02x %02x %02x %02x %02x %02x %02x"
              " %02x %02x %02x %02x %02x %02x %02x %02x",
              b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],
              b[8],b[9],b[10],b[11],b[12],b[13],b[14],b[15]);
  }
  inet_ntop(addr->sa_family, raw, ip_addr, INET6_ADDRSTRLEN);
}

