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
			size_t counter = 0;
			char *p = data;
			char *method = p;
			while (*p != ' ') {
				p++;
				counter++;
			}
			if (counter > 7) {
				req->valid = INVALID;
				return -1;
			}

			*p++ = '\0';
			counter = 0;

			char *path = p;
			while (*p != ' ') {
				p++;
				counter++;
			}

			if (counter > 255) {
				req->valid = INVALID;
				return -1;
			}
			*p++ = '\0';
			counter = 0;

			char* version = p;
			while (*p != ' ' && (*p != '\r' && *(p+ 1) != '\n')) {
				counter++;
				p++;
			}

			if (counter > 15) {
				req->valid = INVALID;
				return -1;
			}
			*p++ = '\0';
			strncpy(req->method, method, sizeof(req->method)-1);
			strncpy(req->path, path, sizeof(req->path)-1);
			strncpy(req->version, version, sizeof(req->version)-1);
            i += 2; 
            break;
        }
    }

    size_t consumed = i; 
    req->header_count = 0;

    // Parse headers
    while (i + 1 < len) {
        if (data[i - 2] == '\r' && data[i - 1] == '\n' &&
            data[i] == '\r' && data[i+1] == '\n') {
            consumed = i + 2; 
            break;
        }

        size_t line_start = i;
        while (i + 1 < len && !(data[i] == '\r' && data[i+1] == '\n')) i++;
        if (i + 1 >= len) { 
			req->valid  = INCOMPLETE;
			return -1;
			break; 
		}

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
	req->valid = COMPLETE;
    return consumed;
}



void http_request_reset(http_request_t *req) {
	req->header_count = 0;
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
	if (strcmp(req->version, "HTTTP/1.1") == 0) {
		if (connection_header && strcasecmp(connection_header, "close") == 0) {
			return false;
		} else {
			return true;
		}
	} else {
		if (connection_header && strcasecmp(connection_header, "keep-alive") == 0) {
			return true;
		}
	}
	return false;
}












