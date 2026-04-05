#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>
#include "core/conn_pool.h"
#include <stddef.h>


#define WORKER_PENDING_CAPACITY 4096

typedef struct {
	int epfd;
	int notify_fd;
	pthread_t thread;
	pthread_mutex_t mutex;

	int pending_fds[WORKER_PENDING_CAPACITY];
	size_t pending_count;
	size_t pending_capacity;
	conn_pool_t pool;
} worker_t;

int worker_init(worker_t *w);
int worker_start(worker_t *w);
void worker_stop(worker_t *w);
int worker_enqueue_client(worker_t *w, int client_fd);

#endif
