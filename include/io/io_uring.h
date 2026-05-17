#include <static/static.h>
#include <sys/types.h>
#include <liburing.h>
#include "core/conn_pool.h"


int io_ring_init(struct io_uring *ring, int queue_depth);

void io_ring_destroy(struct io_uring *ring);

int io_setup_register_buffers(struct io_uring *ring, conn_pool_t *pool);

int io_setup_register_files(struct io_uring* ring, static_file_cache_t *cache); 




