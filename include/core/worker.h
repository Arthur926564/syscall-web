#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>
#include <stddef.h>

typedef struct {
	int epfd;
	int notify_fd;
	pthread_t thread;
	pthread_mutex_t mutex;

	int *pending_fds;
	size_t pending_count;
	size_t pending_capacity;
} worker_t;

int worker_init(worker_t *w);
int worker_start(worker_t *w);
void worker_stop(worker_t *w);
int worker_enqueue_client(worker_t *w, int client_fd);

#endif
