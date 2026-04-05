#ifndef CORE_CONNECTION_H
#define CORE_CONNECTION_H

#include <stddef.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "http/parser.h"
#include "util/buffer.h"

typedef enum {
	CONN_READING_HEADERS,
	CONN_READING_BODY,
	CONN_WRITING,
	CONN_CLOSED
} conn_state_t;


typedef struct connection {
	int fd;
	buffer_t in;
	buffer_t out;
	http_request_t req;
	conn_state_t state;
	int write_offset;
	bool keep_alive;

	int file_fd;
    off_t file_offset; 
    off_t file_size;
    bool sending_file;
} connection_t;


connection_t *connection_create(int fd);

void destroy_connection(int epfd, connection_t *conn);

void connection_destroy(connection_t *connection);

typedef struct conn_pool conn_pool_t;

void process_connection_event(int epfd, struct epoll_event *ev, conn_pool_t *pool);
#endif
