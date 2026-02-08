#include "http/parser.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <fnmatch.h>
#include <string.h>

int next_line_exists(char *data, size_t i, size_t len) {
    if (i + 3 >= len) return 0;
    return !(data[i] == '\r' && data[i+1] == '\n'
             && data[i+2] == '\r' && data[i+3] == '\n');
}


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
	char *data = in->data;
	size_t len = in->len;
	int i = 0;
	while (i + 1 < len) {
		if (data[i] == '\r' && data[i+1] == '\n') {
			char saved = data[i];
			data[i] = '\0';
			int n = sscanf(data, "%7s %255s %15s",
					req->method,
					req->path,
					req->version);
			data[i] = saved;
				
			if (n != 3) {
				return -1;
			}
			break;
		}
		i++;
	}
	int start_of_line = i;
	
	// Now we need to look for all the rest of the header
	// while next line exists
	while (i + 3 < len
			&& next_line_exists(data, i, len)
			&& req->header_count < 32) {
		if (data[i] == '\r' && data[i + 1] == '\n') {
			http_header_t header;
			char current_line[512];
			int line_len = i - start_of_line;
			if (line_len >= sizeof(current_line)) line_len = sizeof(current_line) - 1;
			memcpy(current_line, data + start_of_line, line_len);
			current_line[line_len] = '\0';

			find_header(current_line,  &header);
			req->headers[req->header_count] = header;
			req->header_count += 1;

			i += 2;
			start_of_line = i;
		} else {
			i++;
		}
	}
	
	
	return 1;

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
