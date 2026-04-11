#include "core/connection.h"
#include "util/buffer.h"
#include <stdlib.h>
#include "core/conn_pool.h"

#define MAX_KEEP_CAP (64 * 1024)


connection_t *conn_pool_get(conn_pool_t *p) {
	if (p->top > 0) {
        connection_t *conn = p->stack[--p->top];
        // buffers already allocated and reset by conn_pool_put
        conn->fd = -1;
        conn->state = CONN_READING_HEADERS;
        conn->write_offset = 0;
        conn->keep_alive = false;
        conn->file_fd = -1;
        conn->file_offset = 0;
        conn->file_size = 0;
        conn->sending_file = false;
        return conn;
	}
	connection_t * conn = calloc(1, sizeof(connection_t));
    buffer_init(&conn->in);
    buffer_init(&conn->out);
	return conn;

}

void conn_pool_put(conn_pool_t *p, connection_t *conn) {
	if (p->top < CONN_POOL_CAP) {
		buffer_reset_and_maybe_shrink(&conn->in, MAX_KEEP_CAP);
		buffer_reset_and_maybe_shrink(&conn->out, MAX_KEEP_CAP);
		p->stack[p->top++] = conn;
		return;
	}
	free(conn->in.data);
	free(conn->out.data);
	free(conn);

}
