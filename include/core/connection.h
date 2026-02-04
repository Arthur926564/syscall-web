#include <stddef.h>
#include "util/buffer.h"

typedef struct {
	int fd;
	buffer_t in;
	buffer_t out;
	enum conn_state_t {CONN_READING, CONNN_WRITING, CONN_CLOSED} state;
} connection_t;


connection_t *connection_create(int fd);

void connection_destroy(connection_t *connection);
