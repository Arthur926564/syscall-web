#include "core/connection.h"
#include <stdlib.h>


connection_t *connection_create(int fd) {
	connection_t *connection = malloc(sizeof(connection_t));
	if (!connection) return NULL;
	
	connection->fd = fd;
	connection->state = CONN_CLOSED;
	connection->in.data = NULL;
	connection->in.cap = 0;
	connection->in.len = 0;

	connection->out.data = NULL;
	connection->out.len = 0;
	connection->out.cap = 0;
	return connection;
}

void connection_destroy(connection_t *connection) {
	if (!connection) return;

	free(connection->in.data);
	free(connection->out.data);
	free(connection);
}
