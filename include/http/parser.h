#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "util/buffer.h"

typedef struct {
	char metod[8];
	char path[256];
	char version[16];
} http_request_t;


int http_parse_request(buffer_t *in, http_request_t *req);

int is_static_request(http_request_t *req);

#endif
