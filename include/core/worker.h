#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>
#include "core/conn_pool.h"
#include <stddef.h>


#define WORKER_PENDING_CAPACITY 4096

typedef struct {
	int epfd;
	int listen_fd;
	pthread_t thread;
	conn_pool_t pool;
} worker_t;


int worker_start(worker_t *w);
int worker_init(worker_t *w, int port);

#endif
