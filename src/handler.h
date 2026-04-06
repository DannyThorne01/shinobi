 #ifndef HANDLER_H
  #define HANDLER_H
  #include "http.h"           // needs HttpRequest
  void http_handler(HttpRequest *req, int fd);
  void send_404(int fd);
#endif