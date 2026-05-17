#include "buffer.h"
#include "core/connection.h"
#include <liburing.h>
#include <liburing/io_uring.h>
#include <stdint.h>
#include <stdio.h>
#include "core/worker.h"
#include "io/io_ops.h"

static inline void *pack(connection_t *conn, io_op_t op) {
    return (void *)((uintptr_t)conn | (uintptr_t)op);
}


static inline connection_t *unpack_conn(void *ptr) {
    return (connection_t *)((uintptr_t)ptr & ~(uintptr_t)0x7);
}

static inline io_op_t unpack_op(void *ptr) {
    return (io_op_t)((uintptr_t)ptr & (uintptr_t)0x7);
}


void io_add_accept(worker_t *w) {
	struct io_uring_sqe *sqe = io_uring_get_sqe(&w->ring);
	if (sqe == NULL) {
		fprintf(stderr, "sqe ring full...");
		return;
	}

	io_uring_prep_multishot_accept(sqe, w->listen_fd, NULL, NULL, 0);
	io_uring_sqe_set_data(sqe, NULL);
	io_uring_submit(&w->ring);
}

void io_add_recv(worker_t *w, connection_t *conn) {
	struct io_uring_sqe *sqe = io_uring_get_sqe(&w->ring);
	if (sqe == NULL) {
		fprintf(stderr, "sqe ring full...");
		return;
	}
	io_uring_prep_recv(sqe, conn->fd, write_ptr(&conn->in), write_avail(&conn->in), 0);
	
	io_uring_sqe_set_data(sqe, pack(conn, IO_OP_RECV));
	io_uring_submit(&w->ring);
}

void io_add_send(worker_t *w, connection_t *conn) {
	struct io_uring_sqe *sqe = io_uring_get_sqe(&w->ring);
	if (sqe == NULL) {
		fprintf(stderr, "sqe ring full...");
		return;
	}
	io_uring_prep_send(sqe, conn->fd,
			read_ptr(&conn->out),
			buffer_len(&conn->out),
			0);

	io_uring_sqe_set_data(sqe, pack(conn,  IO_OP_SEND));
	io_uring_submit(&w->ring);
}

void io_add_sendfile(worker_t *w, connection_t *conn) {
	struct io_uring_sqe *sqe = io_uring_get_sqe(&w->ring);
	if (sqe == NULL) {
		fprintf(stderr, "sqe ring full...");
		return;
	}
	io_uring_prep_splice(sqe,
			conn->file_fd, conn->file_offset,
			conn->fd, -1,
			conn->file_size - conn->file_offset,
			0);
	io_uring_sqe_set_data(sqe, pack(conn, IO_OP_SENDFILE));
	io_uring_submit(&w->ring);
}

void io_add_close(worker_t *w, connection_t *conn) {
	struct io_uring_sqe *sqe = io_uring_get_sqe(&w->ring);
	if (sqe == NULL) {
		fprintf(stderr, "sqe ring full...");
		return;
	}
	io_uring_prep_close(sqe, conn->fd);
	io_uring_sqe_set_data(sqe, pack(conn, IO_OP_CLOSE));
	io_uring_submit(&w->ring);
}

