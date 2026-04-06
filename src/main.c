#include <stdlib.h>
#include <string.h>
#include <gdal.h>
#include "utils.h"
#include "server.h"

int main() {
  const char *lvl = getenv("LOG_LEVEL");
  if (lvl && strcmp(lvl, "debug") == 0) {
    g_log_level = LOG_LEVEL_DEBUG;
    LOG_DEBUG("Log level: DEBUG");
  } else {
    LOG_INFO("Log level: INFO  (set LOG_LEVEL=debug for verbose output)");
  }

  GDALAllRegister();
  LOG_INFO("GDAL drivers registered");

  int sockfd = server_init("3490");
  if (sockfd < 0) {
    LOG_INFO("Server init failed");
    return 1;
  }
  return server_run(sockfd);
}
