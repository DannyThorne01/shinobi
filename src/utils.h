#ifndef UTILS_H
  #define UTILS_H

  #include <stdio.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>

  // --- Logging ---

  typedef enum { LOG_LEVEL_INFO = 0, LOG_LEVEL_DEBUG = 1 } LogLevel;

  extern LogLevel g_log_level;

  #define LOG_INFO(fmt, ...)  fprintf(stderr, "[INFO]  " fmt "\n", ##__VA_ARGS__)
  #define LOG_DEBUG(fmt, ...) do { \
      if (g_log_level >= LOG_LEVEL_DEBUG) \
          fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__); \
  } while(0)

    // --- Utils ---
    void addr_to_str(struct sockaddr *addr, char *ip_addr);

  #endif