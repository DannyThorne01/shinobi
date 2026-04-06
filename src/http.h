#ifndef HTTP_H
  #define HTTP_H
  #define HTTP_METHOD_MAX 16                                                                
  #define HTTP_URI_MAX    256
  #define HTTP_PATH_MAX   256                                                                     
  #define HTTP_QUERY_MAX  256
  #define HTTP_HOST_MAX   64 
  #define HTTP_FILENAME_MAX 128


  typedef struct {
    char method[HTTP_METHOD_MAX];
    char route_path[HTTP_PATH_MAX];
    char  file_path[HTTP_FILENAME_MAX];
    char query_string[HTTP_QUERY_MAX];
    char host[HTTP_HOST_MAX];
    char *user_agent; // heap allocated - caller must free
  } HttpRequest;

  typedef enum {
    HTTP_OK = 0,
    HTTP_ERR_MALFORMED = -1,
    HTTP_ERR_METHOD = -2,
    HTTP_ERR_URI = -3,
  } HttpParseResult;

  typedef struct { double min_lon, min_lat, max_lon, max_lat; } BBox;
  typedef struct { double lat, lon; } Latlon;

  HttpParseResult http_parse_request(char *buf, HttpRequest *req);
  int  http_return(const char *type, int file_size, int socket_fd, char *page_buff);
  int  http_parse_bbox(const char *query_string, BBox *bbox);
  void http_parse_latlon(const char *query_string, Latlon *latlon);
#endif