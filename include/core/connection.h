#include <stddef.h>
#include "util/buffer.h"

typedef enum {
	CONN_READING,
	CONNN_WRITING,
	CONN_CLOSED
} conn_state_t;

typedef struct {
	int fd;
	buffer_t in;
	buffer_t out;
	conn_state_t state;
} connection_t;


connection_t *connection_create(int fd);

void connection_destroy(connection_t *connection);
