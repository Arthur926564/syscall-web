
#include <bits/types/struct_iovec.h>
#include <static/static.h>
#include <liburing.h>
#include <stddef.h>
#include <stdio.h>
#include "io/io_uring.h"
#include "core/conn_pool.h"

int io_ring_init(struct io_uring *ring, int queue_depth) {
	if (ring == NULL) {
		perror("given a null pointer for ring");
		return -1;
	}
	int err = io_uring_queue_init(queue_depth, ring, 0);
	if (err < 0) {
		return err;
	}
	return 0;
}

void io_ring_destroy(struct io_uring *ring) {
	if (ring == NULL) {
		return;
	}
	io_uring_queue_exit(ring);
}

int io_setup_register_buffers(struct io_uring *ring, conn_pool_t *pool) {
	struct iovec vecs[CONN_POOL_CAP];

	for (int i = 0; i < CONN_POOL_CAP; i++) {
		vecs[i].iov_base = pool->conns[i].in.data;
		vecs[i].iov_len = pool->conns[i].in.cap;
	}
	return io_uring_register_buffers(ring, vecs, CONN_POOL_CAP);

}
int io_setup_register_files(struct io_uring* ring, static_file_cache_t *cache) {
	int fds[cache->count];
	for (size_t i = 0; i < cache->count; i++) {
		fds[i] = cache->entries[i].fd;
	}

	return io_uring_register_files(ring, fds, cache->count);
}



