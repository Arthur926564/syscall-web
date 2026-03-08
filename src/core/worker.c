
#include "core/worker.h"
#include "core/server.h"
#include "core/connection.h"
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <asm-generic/errno.h>
#include <unistd.h>



int worker_init(worker_t *w) {
    w->epfd = epoll_create1(0);
    if (w->epfd == -1) {
        perror("epoll_create1");
        return -1;
    }

    w->notify_fd = eventfd(0, EFD_NONBLOCK);
    if (w->notify_fd == -1) {
        perror("eventfd");
        return -1;
    }

    pthread_mutex_init(&w->mutex, NULL);

    w->pending_count = 0;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = NULL;   

    if (epoll_ctl(w->epfd, EPOLL_CTL_ADD, w->notify_fd, &ev) == -1) {
        perror("epoll_ctl notify_fd");
        return -1;
    }

    return 0;
}

static void worker_drain_pending(worker_t *w) {
    uint64_t val;

    read(w->notify_fd, &val, sizeof(val));

    pthread_mutex_lock(&w->mutex);

    for (size_t i = 0; i < w->pending_count; i++) {

        int client_fd = w->pending_fds[i];

        connection_t *conn = calloc(1, sizeof(connection_t));
        if (!conn) {
            perror("calloc");
            close(client_fd);
            continue;
        }

        conn->fd = client_fd;
        buffer_init(&conn->in);
        buffer_init(&conn->out);
        conn->state = CONN_READING_HEADERS;

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = conn;

        if (epoll_ctl(w->epfd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            perror("epoll_ctl ADD client");
            close(client_fd);
            free(conn);
            continue;
        }
    }

    w->pending_count = 0;

    pthread_mutex_unlock(&w->mutex);
}


static void *worker_loop(void *arg) {
	worker_t *w = arg;
	struct epoll_event events[128];

	while (1) {
	
		int n = epoll_wait(w->epfd, events, 128, -1);
		if (n == -1) {
			perror("epoll_wait");
			continue;
		}

		for (size_t i = 0;  i < n; i++) {
			if (events[i].data.ptr == NULL) {
				worker_drain_pending(w);
			} else {
				process_connection_event(w->epfd, &events[i]);
			}
		}
	}
}

int worker_start(worker_t *w) {
	int rc = pthread_create(&w->thread, NULL, worker_loop, w);

	if (rc != 0) {
		perror("pthread_create");
		return -1;
	}
	return 0;
}




int worker_enqueue_client(worker_t *w, int client_fd) {
    pthread_mutex_lock(&w->mutex);

    w->pending_fds[w->pending_count++] = client_fd;

    pthread_mutex_unlock(&w->mutex);

    uint64_t one = 1;
    write(w->notify_fd, &one, sizeof(one));

    return 0;
}
