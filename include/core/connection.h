#ifndef CORE_CONNECTION_H
#define CORE_CONNECTION_H

#include <stddef.h>
#include "http/parser.h"
#include "util/buffer.h"

typedef enum {
	CONN_READING_HEADERS,
	CONN_READING_BODY,
	CONN_WRITING,
	CONN_CLOSED
} conn_state_t;

typedef struct {
	int fd;
	buffer_t in;
	buffer_t out;
	http_request_t req;
	conn_state_t state;
} connection_t;


connection_t *connection_create(int fd);

void connection_destroy(connection_t *connection);

#endif
