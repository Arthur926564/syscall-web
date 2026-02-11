#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "util/buffer.h"
#include <stdbool.h>


typedef struct {
	char key[32];
	char value[256];
} http_header_t;

typedef struct {
	char method[8];
	char path[256];
	char version[16];
	http_header_t headers[32];
	int header_count;
} http_request_t;

const char * get_header(http_request_t *req, const char* key);


int http_parse_request(buffer_t *in, http_request_t *req);

void http_request_reset(http_request_t *req);

int is_static_request(http_request_t *req);

bool keep_alive(http_request_t *req);

#endif
