
#include "core/connection.h"
#include "core/worker.h"
#include <liburing.h>
#include <stdint.h>
#include "io/io_uring.h"



// this is what claude advises me to do so i'll see how it goes
typedef struct {
	connection_t *conn;
	uint8_t op;
} io_tag_t;


typedef enum {
	IO_OP_ACCEPT,
	IO_OP_RECV,
	IO_OP_SEND,
	IO_OP_SPLICE_IN,
	IO_OP_SENDFILE,
	IO_OP_CLOSE
} io_op_t;

io_op_t unpack_op(void *ptr);
void *pack(connection_t *conn, io_op_t op);
connection_t *unpack_conn(void *ptr);
void io_add_accept(worker_t *w);
void io_add_recv(worker_t *w, connection_t *conn);
void io_add_send(worker_t *w, connection_t *conn);
void io_add_sendfile(worker_t *w, connection_t *conn);
void io_add_close(worker_t *w, connection_t *conn);
void conn_maybe_close(worker_t *w, connection_t *conn);
