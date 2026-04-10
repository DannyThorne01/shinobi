#include "http.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>

HttpParseResult http_parse_request(char *buf, HttpRequest *req) {
  char *saveptr, *res;

  char *line = strtok_r(buf, "\r\n", &saveptr);
  if (!line) {
    LOG_DEBUG("Parse error: empty request");
    return HTTP_ERR_MALFORMED;
  }

  // first line of the buf - GET /get/result.tif HTTP/1.1
  char *method = strtok_r(line, " ", &res);
  char *uri    = strtok_r(NULL, " ", &res);
  if (!method || !uri) {
    LOG_DEBUG("Parse error: missing method or URI");
    return HTTP_ERR_MALFORMED;
  }

  LOG_DEBUG("Request line: method='%s' uri='%s'", method, uri);

  if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0) {
    LOG_DEBUG("Rejected method: %s", method);
    return HTTP_ERR_METHOD;
  }
  if (strstr(uri, "..") || strlen(uri) > 200) {
    LOG_DEBUG("Rejected URI: %s", uri);
    return HTTP_ERR_URI;
  }

  snprintf(req->method, sizeof(req->method), "%s", method);

  char uri_copy[HTTP_URI_MAX];
  strncpy(uri_copy, uri, sizeof(uri_copy) - 1);
  uri_copy[sizeof(uri_copy) - 1] = '\0';

  // URI format: /<route>/<filename>?<query>  e.g. /get/file.tiff?bbox=0,0,1,1
  char *query_saveptr;
  char *path     = strtok_r(uri_copy, "/", &query_saveptr);  // "get"
  char *filename = strtok_r(NULL,     "?", &query_saveptr);  // "file.tiff"
  char *query    = strtok_r(NULL,     "",  &query_saveptr);  // "bbox=0,0,1,1"

  // route_path: "/" for bare root, "/<segment>" otherwise
  if (path) {
    snprintf(req->route_path, sizeof(req->route_path), "/%s", path);
  } else {
    snprintf(req->route_path, sizeof(req->route_path), "/");
  }

  // file_path: "html/<filename>" or empty for root
  if (filename) {
    snprintf(req->file_path, sizeof(req->file_path), "html/%s", filename);
  } else {
    req->file_path[0] = '\0';
  }

  if (query) {
    snprintf(req->query_string, sizeof(req->query_string), "%s", query);
  } else {
    req->query_string[0] = '\0';
  }

  LOG_DEBUG("Parsed: route='%s' file='%s' query='%s'",
            req->route_path, req->file_path, req->query_string);

  // Parse headers
  char *token = strtok_r(NULL, "\r\n", &saveptr);
  //Host: localhost:3490-- "host" "loacl...."
  //User-Agent: 
  while (token != NULL) {// while there is either a host or agent line 
    char *tok1 = strtok_r(token, ":", &saveptr);
    if (*saveptr == ' ') saveptr++;
    if (strncmp(tok1, "Host", 4) == 0) {
      snprintf(req->host, sizeof(req->host), "%s",saveptr);
      LOG_DEBUG("Host: %s", req->host);
    } 
    if (strncmp(tok1, "User-Agent", 10) == 0) {
      req->user_agent = calloc(strlen(saveptr) + 1, sizeof(char));
      strncpy(req->user_agent, saveptr, strlen(saveptr));
      LOG_DEBUG("User-Agent: %s", req->user_agent);
    }
    token = strtok_r(NULL, "\r\n", &saveptr);
  }
  return HTTP_OK;
}

int http_return(const char *type, int file_size, int socket_fd, char *page_buff) {
  char header[300];
  int hlen = snprintf(header, sizeof(header),
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: %s\r\n"
      "Content-Length: %d\r\n"
      "Connection: close\r\n"
      "\r\n",
      type, file_size);

  LOG_DEBUG("Sending response: Content-Type=%s Content-Length=%d", type, file_size);

  if (send(socket_fd, header, hlen, 0) == -1) {
    LOG_DEBUG("http_return: failed to send headers");
    return -1;
  }

  ssize_t total = 0;
  while (total < file_size) {
    ssize_t n = send(socket_fd, page_buff + total, file_size - total, 0);
    if (n == -1) {
      LOG_DEBUG("http_return: send failed at offset %zd", total);
      return -1;
    }
    total += n;
  }

  LOG_DEBUG("http_return: sent %zd bytes", total);
  return 0;
}

int http_parse_bbox(const char *query_string, BBox *bbox) {
  char buf[HTTP_QUERY_MAX];
  strncpy(buf, query_string, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  char *saveptr;
  strtok_r(buf, "=", &saveptr);                             
  bbox->min_lon = atof(strtok_r(NULL, ",", &saveptr));
  bbox->min_lat = atof(strtok_r(NULL, ",", &saveptr));
  bbox->max_lon = atof(strtok_r(NULL, ",", &saveptr));
  bbox->max_lat = atof(strtok_r(NULL, ",", &saveptr));

  LOG_DEBUG("BBox: min_lon=%f min_lat=%f max_lon=%f max_lat=%f",
            bbox->min_lon, bbox->min_lat, bbox->max_lon, bbox->max_lat);
  return 0;
}

void http_parse_latlon(char *query_string, Latlon *latlon) {
  char *lon;
  char *lat = strtok_r(query_string, "&", &lon);
  strtok_r(lat, "=", &lat);
  strtok_r(lon, "=", &lon);
  latlon->lat = atof(lat);
  latlon->lon = atof(lon);
  LOG_DEBUG("LatLon: lat=%f lon=%f", latlon->lat, latlon->lon);
}
