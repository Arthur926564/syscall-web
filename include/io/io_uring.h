#include <signal.h>
#include <sys/types.h>
#include <liburing.h>


int io_ring_init(struct io_uring *ring, int queue_depth);

void io_ring_destroy(struct io_uring *ring);

void io_ring_register_buffers(struct io_uring *ring, ...);

void io_ring_register_files(struct io_uring* ring, ...);




