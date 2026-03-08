#include "core/connection.h"
#include "http/handler.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>


connection_t *connection_create(int fd) {
	connection_t *connection = malloc(sizeof(connection_t));
	if (!connection) return NULL;
	
	connection->fd = fd;
	connection->state = CONN_READING_HEADERS;
	connection->in.data = NULL;
	connection->in.cap = 0;
	connection->in.start = 0;
	connection->in.end = 0;

	connection->out.data = NULL;
	connection->out.cap = 0;
	connection->out.start = 0;
	connection->out.end = 0;
	return connection;
}

void connection_destroy(connection_t *connection) {
	if (!connection) return;

	free(connection->in.data);
	free(connection->out.data);
	free(connection);
}

void destroy_connection(int epfd, connection_t *conn) {
	epoll_ctl(epfd, EPOLL_CTL_DEL, conn->fd, NULL);
	close(conn->fd);
	free(conn->in.data);
	free(conn->out.data);
	free(conn);
}

void process_connection_event(int epfd, struct epoll_event *ev) {
	connection_t *conn = ev->data.ptr;
	if (ev->events & (EPOLLERR | EPOLLHUP)) {
		conn->state = CONN_CLOSED;
	}

	if (ev->events & EPOLLIN) {
		handle_read(epfd, conn);
	}
	
	if (conn->state != CONN_CLOSED && ev->events & EPOLLOUT) {
		handle_write(epfd, conn);
	}

	if (conn->state == CONN_CLOSED) {
		destroy_connection(epfd, conn);
	}
}
