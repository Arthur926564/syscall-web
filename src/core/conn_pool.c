#include "core/connection.h"
#include "util/buffer.h"
#include <stdlib.h>
#include "core/conn_pool.h"

#define MAX_KEEP_CAP (64 * 1024)

void conn_pool_init(conn_pool_t *p) {
	p->top = 0;
	for (int i = 0; i < CONN_POOL_CAP; i++) {
		buffer_init(&p->conns[i].in);
		buffer_init(&p->conns[i].out);
		p->stack[p->top++] = &p->conns[i];
	}
}


connection_t *conn_pool_get(conn_pool_t *p) {
	if (p->top > 0) {
        connection_t *conn = p->stack[--p->top];
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
	return NULL;

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
