
#include "core/connection.h"
#include <liburing.h>
#include <stdint.h>


// this is what claude advises me to do so i'll see how it goes
typedef struct {
	connection_t *conn;
	uint8_t op;
} io_tag_t;

void io_add_accept(struct io_uring *ring, int listen_fd);
void io_add_recv(struct io_uring *ring, connection_t *conn);
void io_add_send(struct io_uring *ring, connection_t *conn);
void io_add_sendfile(struct io_uring *ring, connection_t *conn);
void io_add_close(struct io_uring *ring, connection_t *conn);

