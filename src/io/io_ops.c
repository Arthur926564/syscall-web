#include "core/conn_pool.h"
#include "util/buffer.h"
#include "core/connection.h"
#include <fcntl.h>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "core/worker.h"
#include "io/io_ops.h"
#define MAX_KEEP_CAP (64 * 1024)

void *pack(connection_t *conn, io_op_t op) {
    return (void *)((uintptr_t)conn | (uintptr_t)op);
}

connection_t* unpack_conn(void *ptr) {
    return (connection_t *)((uintptr_t)ptr & ~(uintptr_t)0x7);
}

io_op_t unpack_op(void *ptr) {
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
}

void io_add_recv(worker_t *w, connection_t *conn) {
	if (conn->closing) return;
	struct io_uring_sqe *sqe = io_uring_get_sqe(&w->ring);
	if (sqe == NULL) {
		fprintf(stderr, "sqe ring full...");
		return;
	}
	conn->inflight++;
	io_uring_prep_recv(sqe, conn->fd, write_ptr(&conn->in), write_avail(&conn->in), 0);
	
	io_uring_sqe_set_data(sqe, pack(conn, IO_OP_RECV));
}

void io_add_send(worker_t *w, connection_t *conn) {

	if (conn->closing) {
		return;
	}
	struct io_uring_sqe *sqe = io_uring_get_sqe(&w->ring);
	if (sqe == NULL) {
		io_add_close(w, conn);
		return;
	}
	char  *buf = read_ptr(&conn->out) + conn->write_offset;
    size_t len = buffer_len(&conn->out) - conn->write_offset;
	io_uring_prep_send(sqe, conn->fd, buf, len, 0);

	conn->inflight++;
	io_uring_sqe_set_data(sqe, pack(conn,  IO_OP_SEND));
}

// Queues an IORING_OP_SEND of the remaining file bytes (mmap'd at startup
// in the static cache). SPLICE is not used: on this kernel, io_uring splice
// is per-call capped to 2 pages once the pipe-user-pages soft limit is hit,
// which turns a 100 KiB file into ~13 async-worker dispatches. A single
// SEND of the mmap'd buffer is one SQE, one CQE, no pipes involved.
void io_add_sendfile(worker_t *w, connection_t *conn) {
	if (conn->closing) return;

	size_t remaining = (size_t)(conn->file_size - conn->file_offset);
	if (remaining == 0 || conn->file_data == NULL) return;

	struct io_uring_sqe *sqe = io_uring_get_sqe(&w->ring);
	if (sqe == NULL) { io_add_close(w, conn); return; }

	io_uring_prep_send(sqe, conn->fd,
	                   conn->file_data + conn->file_offset,
	                   remaining, 0);
	io_uring_sqe_set_data(sqe, pack(conn, IO_OP_SENDFILE));
	conn->inflight++;
}

void conn_maybe_close(worker_t *w, connection_t *conn) {
    if (conn->inflight > 0) return;  
    if (!conn->closing) return;     
    conn->closing  = false;
    conn->inflight = 0;
    conn->fd       = -1;
    buffer_reset_and_maybe_shrink(&conn->in,  MAX_KEEP_CAP);
    buffer_reset_and_maybe_shrink(&conn->out, MAX_KEEP_CAP);
    conn->write_offset = 0;
    conn_pool_put(&w->pool, conn);
}

void io_add_close(worker_t *w, connection_t *conn) {
    if (conn->closing) return;  
    conn->closing = true;
	if (conn->fd >= 0) {
    	close(conn->fd);
		conn->fd = -1;
	}
    conn_maybe_close(w, conn);  
}
