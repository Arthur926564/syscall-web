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
    size_t len  = buffer_len(in);
    size_t i    = 0;

    // --- request line: method ---
    size_t start = i;
    while (i < len && data[i] != ' ') i++;
    if (i >= len) { req->valid = INCOMPLETE; return 0; }
    if (i - start > sizeof(req->method) - 1) { req->valid = INVALID; return -1; }
    memcpy(req->method, data + start, i - start);
    req->method[i - start] = '\0';
    i++;  // skip space

    // --- path ---
    start = i;
    while (i < len && data[i] != ' ') i++;
    if (i >= len) { req->valid = INCOMPLETE; return 0; }
    if (i - start > sizeof(req->path) - 1) { req->valid = INVALID; return -1; }
    memcpy(req->path, data + start, i - start);
    req->path[i - start] = '\0';
    i++;  // skip space

    // --- version ---
    start = i;
    while (i + 1 < len && !(data[i] == '\r' && data[i+1] == '\n')) i++;
    if (i + 1 >= len) { req->valid = INCOMPLETE; return 0; }
    if (i - start > sizeof(req->version) - 1) { req->valid = INVALID; return -1; }
    memcpy(req->version, data + start, i - start);
    req->version[i - start] = '\0';
    i += 2;  // skip \r\n

    // --- headers ---
    req->header_count = 0;
    while (i + 1 < len) {
        // blank line = end of headers (\r\n on its own)
        if (data[i] == '\r' && data[i+1] == '\n') {
            req->valid = COMPLETE;
            return (int)(i + 2);
        }

        // find end of this header line
        start = i;
        while (i + 1 < len && !(data[i] == '\r' && data[i+1] == '\n')) i++;
        if (i + 1 >= len) { req->valid = INCOMPLETE; return 0; }

        // parse key: value
        if (req->header_count < 32) {
            char *line     = data + start;
            size_t line_len = i - start;
            char *colon    = memchr(line, ':', line_len);
            if (colon) {
                http_header_t *h = &req->headers[req->header_count];
                size_t key_len = colon - line;
                colon++;
                while (*colon == ' ') colon++;  // trim leading space
                size_t val_len = (data + i) - colon;

                // copy with bounds check — no strncpy needed
                key_len = key_len < sizeof(h->key) - 1   ? key_len : sizeof(h->key) - 1;
                val_len = val_len < sizeof(h->value) - 1 ? val_len : sizeof(h->value) - 1;
                memcpy(h->key,   line,  key_len); h->key[key_len]   = '\0';
                memcpy(h->value, colon, val_len); h->value[val_len] = '\0';
                req->header_count++;
            }
        }
        i += 2;  // skip \r\n
    }

    req->valid = INCOMPLETE;
    return 0;
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



bool keep_alive(http_request_t *req) {
    const char *conn = get_header(req, "Connection");
    // HTTP/1.1 is keepalive by default
    if (strlen(req->version) == 8 && memcmp(req->version, "HTTP/1.1", 8) == 0) {
        return !(conn && strcasecmp(conn, "close") == 0);
    }
    // HTTP/1.0 needs explicit keepalive
    return conn && strcasecmp(conn, "keep-alive") == 0;
}


int is_static_request(http_request_t *req) {
    return strlen(req->method) == 3 && memcmp(req->method, "GET", 3) == 0;
}
