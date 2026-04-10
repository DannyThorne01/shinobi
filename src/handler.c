#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <gdal.h>
#include "utils.h"
#include "http.h"
#include "handler.h"

void send_404(int fd) {
  const char *r404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
  send(fd, r404, strlen(r404), 0);
  LOG_DEBUG("Sent 404 on fd=%d", fd);
}

void http_handler(HttpRequest *http_request, int new_fd) {
  FILE  *fptr      = NULL;
  VSILFILE *vsi_fptr = NULL;
  vsi_l_offset  vsi_file_size =0;
  long   file_size = 0;

  char  *page_buf  = NULL;

  LOG_INFO("Route='%s' file='%s' query='%s'",
           http_request->route_path,
           http_request->file_path,
           http_request->query_string);

  if (strcmp(http_request->route_path, "/") == 0) {

    LOG_DEBUG("Serving html/index.html");
    fptr = fopen("html/index.html", "r");
    if (fptr == NULL) {
      LOG_DEBUG("Could not open html/index.html");
      send_404(new_fd);
      return;
    }

    fseek(fptr, 0, SEEK_END);
    file_size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    LOG_DEBUG("index.html: %ld bytes", file_size);

    page_buf = calloc(file_size + 1, sizeof(char));
    if (page_buf == NULL) {
      LOG_DEBUG("calloc failed for index.html");
      fclose(fptr);
      return;
    }
    fread(page_buf, sizeof(char), file_size, fptr);
    page_buf[file_size] = '\0';
    fclose(fptr);

    http_return("text/html", file_size, new_fd, page_buf);
    LOG_INFO("Served index.html (%ld bytes)", file_size);

  } else if (strcmp(http_request->route_path, "/get") == 0) {

    const char *filepath = http_request->file_path;
    LOG_DEBUG("Serving raster: %s", filepath);

    GDALDatasetH ds = GDALOpen(filepath, GA_ReadOnly);
    if (ds == NULL) {
      LOG_DEBUG("GDALOpen failed: %s", filepath);
      send_404(new_fd);
      return;
    }
    GDALClose(ds);
    LOG_DEBUG("GDAL validated: %s", filepath);

    fptr = fopen(filepath, "rb");
    if (fptr == NULL) {
      LOG_DEBUG("fopen failed: %s", filepath);
      send_404(new_fd);
      return;
    }

    fseek(fptr, 0, SEEK_END);
    file_size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    LOG_DEBUG("File size: %ld bytes", file_size);

    page_buf = calloc(file_size + 1, sizeof(char));
    if (page_buf == NULL) {
      LOG_DEBUG("calloc failed for %s", filepath);
      fclose(fptr);
      return;
    }
    fread(page_buf, sizeof(char), file_size, fptr);
    page_buf[file_size] = '\0';
    fclose(fptr);

    http_return("image/tiff", file_size, new_fd, page_buf);
    LOG_INFO("Served %s (%ld bytes)", filepath, file_size);

  } else if (strcmp(http_request->route_path, "/bbox") == 0) {
    const char *filepath = http_request->file_path;
    LOG_DEBUG("Serving raster: %s", filepath);
    GDALDatasetH ds = GDALOpen(filepath, GA_ReadOnly);
    if (ds == NULL) {
      LOG_DEBUG("GDALOpen failed: %s", filepath);
      send_404(new_fd);
      return;
    }
    // get the bbox
    Bbox bbox = {0};
    http_parse_bbox(http_request->query_string, &bbox);
    char *dest_filepath = "/vsimem/output.tiff";
    char xmin[32];snprintf(xmin, sizeof(xmin), "%f",bbox.min_lon);
    char xmax[32];snprintf(xmax, sizeof(xmax), "%f",bbox.max_lon);
    char ymin[32];snprintf(ymin, sizeof(ymin), "%f",bbox.max_lat);
    char ymax[32];snprintf(ymax, sizeof(ymax), "%f",bbox.min_lat);
    char *args[] = {"-projwin", xmin, xmax, ymin, ymax, NULL};
    GDALTranslateOptions *gdal_args = GDALTranslateOptionsNew(args,NULL); 
    GDALDatasetH ds_clipped = GDALTranslate(dest_filepath, ds, gdal_args,NULL);
    GDALTranslateOptionsFree(gdal_args);
    if (ds_clipped  == NULL){
      LOG_DEBUG("GDAL CLIP failed: %s", dest_filepath);
      send_404(new_fd);
      GDALClose(ds);
      return;
    }
    GDALClose(ds);
    LOG_DEBUG("GDAL validated: %s", filepath);

    vsi_fptr = VSIFOpenL(dest_filepath, "rb");
    if (vsi_fptr == NULL) {
      LOG_DEBUG("Clip was successful but couldn't open clipped file: %s", dest_filepath);
      send_404(new_fd);
      GDALClose(ds_clipped);
      return;
    }
    VSIFSeekL(vsi_fptr, 0, SEEK_END);
    vsi_file_size = VSIFTellL(vsi_fptr);
    VSIFSeekL(vsi_fptr, 0, SEEK_SET);
    LOG_DEBUG("File size: %llu bytes", vsi_file_size);
    page_buf = calloc(vsi_file_size + 1, sizeof(char));
    if (page_buf == NULL) {
      LOG_DEBUG("calloc failed for %s", filepath);
      GDALClose(ds_clipped);
      VSIFCloseL(vsi_fptr);
      return;
    }
    VSIFReadL(page_buf, sizeof(char), vsi_file_size, vsi_fptr);
    page_buf[vsi_file_size] = '\0';
    VSIFCloseL(vsi_fptr);
    GDALClose(ds_clipped);
    VSIUnlink(dest_filepath);

    http_return("image/tiff", vsi_file_size, new_fd, page_buf);
    LOG_INFO("Served %s (%llu bytes)", filepath, vsi_file_size);
  }
  else {
    LOG_DEBUG("Unknown route: %s", http_request->route_path);
    send_404(new_fd);
  }

  free(page_buf);
}
