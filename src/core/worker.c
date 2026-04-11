#include "core/worker.h"
#include "core/conn_pool.h"
#include "core/server.h"
#include "core/connection.h"
#include "net/tcp.h"
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "core/server.h"
#include "core/worker.h"
#include "static/static.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>



#ifndef MAX_PENDING
#define MAX_PENDING 4096
#endif

static void *worker_loop(void *arg);




static void worker_accept(worker_t *w) {
    while (1) {
        struct sockaddr client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept4(w->listen_fd, &client_addr, &addrlen, SOCK_NONBLOCK);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break; // done for now
            if (errno == EINTR) continue;
            perror("accept4");
            break;
        }
        connection_t *conn = conn_pool_get(&w->pool);
        if (!conn) { close(client_fd); continue; }

        conn->fd    = client_fd;
        conn->state = CONN_READING_HEADERS;

        struct epoll_event ev = {
            .events   = EPOLLIN | EPOLLET | EPOLLRDHUP,
            .data.ptr = conn,
        };
        if (epoll_ctl(w->epfd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
			perror("epoll_ctl_add");
            close(client_fd);
            conn_pool_put(&w->pool, conn);
        }
    }
}



int worker_init(worker_t *w, int port) {
    w->epfd = epoll_create1(0);
    if (w->epfd < 0) { perror("epoll_create1"); return -1; }

    w->listen_fd = tcp_listen_reuseport(port);
    if (w->listen_fd < 0) { perror("tcp_listen_reuseport"); return -1; }

    struct epoll_event ev = {
        .events   = EPOLLIN,
        .data.ptr = NULL,  
    };
    if (epoll_ctl(w->epfd, EPOLL_CTL_ADD, w->listen_fd, &ev) < 0) {
        perror("epoll_ctl listen_fd");
        return -1;
    }

    return 0;
}

static void *worker_loop(void *arg) {
    worker_t *w = arg;
    struct epoll_event events[128];

    while (1) {
        int n = epoll_wait(w->epfd, events, 128, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            continue;
        }
        for (int i = 0; i < n; i++) {
            if (events[i].data.ptr == NULL) {
                worker_accept(w);          // listen_fd fired
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


