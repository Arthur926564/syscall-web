#include "core/worker.h"
#include "core/server.h"
#include "core/connection.h"
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>

#ifndef MAX_PENDING
#define MAX_PENDING 4096
#endif

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

    if (pthread_mutex_init(&w->mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        close(w->notify_fd);
        close(w->epfd);
        return -1;
    }

    w->pending_count = 0;

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.ptr = NULL;

    if (epoll_ctl(w->epfd, EPOLL_CTL_ADD, w->notify_fd, &ev) == -1) {
        perror("epoll_ctl notify_fd");
        close(w->notify_fd);
        close(w->epfd);
        return -1;
    }

    return 0;
}

static void worker_drain_pending(worker_t *w) {
    uint64_t val;
    int local_fds[MAX_PENDING];
    size_t local_count = 0;

    ssize_t n = read(w->notify_fd, &val, sizeof(val));
    if (n == -1 && errno != EAGAIN) {
        perror("read notify_fd");
    }

    pthread_mutex_lock(&w->mutex);

    local_count = w->pending_count;
    if (local_count > 0) {
        memcpy(local_fds, w->pending_fds, local_count * sizeof(int));
        w->pending_count = 0;
    }

    pthread_mutex_unlock(&w->mutex);

    for (size_t i = 0; i < local_count; i++) {
        int client_fd = local_fds[i];

        connection_t *conn = conn_pool_get(&w->pool);
        if (!conn) {
            perror("calloc");
            close(client_fd);
            continue;
        }

        conn->fd = client_fd;
        conn->state = CONN_READING_HEADERS;

        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = conn;

        if (epoll_ctl(w->epfd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            perror("epoll_ctl ADD client");
            close(client_fd);
			conn_pool_put(&w->pool, conn);
            continue;
        }
    }
}

static void *worker_loop(void *arg) {
    worker_t *w = arg;
    struct epoll_event events[128];

    while (1) {
        int n = epoll_wait(w->epfd, events, 128, -1);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            continue;
        }

        for (int i = 0; i < n; i++) {
            if (events[i].data.ptr == NULL) {
                worker_drain_pending(w);
            } else {
                process_connection_event(w->epfd, &events[i], &w->pool);
            }
        }
    }

    return NULL;
}

int worker_start(worker_t *w) {
    int rc = pthread_create(&w->thread, NULL, worker_loop, w);
    if (rc != 0) {
        errno = rc;
        perror("pthread_create");
        return -1;
    }
    return 0;
}

int worker_enqueue_client(worker_t *w, int client_fd) {
    bool need_wakeup = false;

    pthread_mutex_lock(&w->mutex);

    if (w->pending_count >= MAX_PENDING) {
        pthread_mutex_unlock(&w->mutex);
        close(client_fd);
        return -1;
    }

    if (w->pending_count == 0) {
        need_wakeup = true;
    }

    w->pending_fds[w->pending_count++] = client_fd;

    pthread_mutex_unlock(&w->mutex);

    if (need_wakeup) {
        uint64_t one = 1;
        ssize_t n = write(w->notify_fd, &one, sizeof(one));
        if (n == -1 && errno != EAGAIN) {
            perror("write notify_fd");
            return -1;
        }
    }

    return 0;
}
