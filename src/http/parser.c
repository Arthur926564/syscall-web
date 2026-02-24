#include "http/parser.h"
#include "util/buffer.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <fnmatch.h>
#include <string.h>
#include <stdbool.h>


int find_header(char *line, http_header_t *header) {
	char *colon = strchr(line, ':');

	if (!colon) return -1;
	*colon = '\0';
	char *key = line;
	char *value = colon + 1;

	while (*value == ' ') value++;
	strncpy(header->key, key, sizeof(header->key) - 1);
	strncpy(header->value, value, sizeof(header->value) - 1);

	header->key[sizeof(header->key) - 1] = '\0';
	header->value[sizeof(header->value) - 1] = '\0';


	return 0;
}

int http_parse_request(buffer_t *in, http_request_t *req) {
    char *data = read_ptr(in);
    size_t len = buffer_len(in);
    size_t i = 0;

    // Parse request line
    for (; i + 1 < len; i++) {
        if (data[i] == '\r' && data[i+1] == '\n') {
            data[i] = '\0';
            if (sscanf(data, "%7s %255s %15s",
                       req->method,
                       req->path,
                       req->version) != 3) {
                return -1;
            }
            data[i] = '\r';
            i += 2; 
            break;
        }
    }

    size_t consumed = i; 
    req->header_count = 0;

    // Parse headers
    while (i + 3 < len) {
        if (data[i] == '\r' && data[i+1] == '\n' &&
            data[i+2] == '\r' && data[i+3] == '\n') {
            consumed = i + 4; 
            break;
        }

        size_t line_start = i;
        while (i + 1 < len && !(data[i] == '\r' && data[i+1] == '\n')) i++;
        if (i + 1 >= len) break; 

        char saved = data[i];
        data[i] = '\0';

        if (req->header_count < 32) {
            http_header_t header;
            if (find_header(data + line_start, &header) == 0) {
                req->headers[req->header_count++] = header;
            }
        }

        data[i] = saved;
        i += 2; 
    }
    return i + 2;
}



void http_request_reset(http_request_t *req) {
	req->header_count = 0;
	memset(req->method, 0, sizeof(req->method));
	memset(req->path, 0, sizeof(req->path));
	memset(req->version, 0, sizeof(req->version));
	for (size_t i = 0; i < 32; i++) {
		req->headers[i].key[0] = '\0';
		req->headers[i].value[0] = '\0';
	}
}

const char * get_header(http_request_t *req, const char *key) {
	if (!req || !key) return NULL;
	for (size_t i = 0; i < req->header_count; i++) {
		if (strcasecmp(req->headers[i].key, key) == 0) {
			return req->headers[i].value;
		}
	}
	return NULL;
}

int is_static_request(http_request_t *req) {
	return strcmp(req->method, "GET") == 0;
}


bool keep_alive(http_request_t *req) {
	const char *connection_header = get_header(req, "Connection");
	if (connection_header && strcasecmp(connection_header, "keep-alive") == 0) {
		return true;
	}
	return false;
}












