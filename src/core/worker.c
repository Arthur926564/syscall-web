#include "core/worker.h"
#include "core/conn_pool.h"
#include "core/server.h"
#include "core/connection.h"
#include "net/tcp.h"
#include <fcntl.h>
#include <liburing.h>
#include <liburing/io_uring.h>
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
#include "io/io_uring.h"
#include "io/io_ops.h"
#include "util/buffer.h"
#include "http/http_response.h"
#include "http/handler.h"

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
        if (!conn) { 
			fprintf(stderr, "pool echausted\n");
			close(client_fd);
			continue; 
		}

        conn->fd    = client_fd;
        conn->state = CONN_READING_HEADERS;

        struct epoll_event ev = {
            .events   = EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP,
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
	conn_pool_init(&w->pool);
	

	if (io_ring_init(&w->ring, 256) < 0) {
		return -1;
	}
    w->listen_fd = tcp_listen_reuseport(port);
    if (w->listen_fd < 0) { perror("tcp_listen_reuseport"); return -1; }
	pipe2(w->pipe_fd, O_NONBLOCK);

	io_add_accept(w);

	return 0;
}

static void *worker_loop(void *arg) {
    worker_t *w = arg;
	struct io_uring_cqe *cqe;

    while (1) {
		int ret = io_uring_submit_and_wait(&w->ring, 1);
		if (ret < 0) {
			if (ret == -EINTR) {
				continue;
			}
            fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret));
            continue; // This is full claude need to understand this before
		}

		unsigned head;
		unsigned count = 0;
		io_uring_for_each_cqe(&w->ring, head, cqe) {
			void *data = io_uring_cqe_get_data(cqe);
			int res = cqe->res;
			uint32_t flags = cqe->flags;

			if (data == NULL) {
				handle_accept(w, res, flags);
			} else {
				io_op_t op = unpack_op(data);
				connection_t *conn = unpack_conn(data);

				switch (op) {
					case IO_OP_RECV:  handle_recv(w, conn, res); break;
					case IO_OP_SEND: handle_send(w, conn,  res); break;
					case IO_OP_SENDFILE: handle_sendfile(w, conn,  res); break;
					case IO_OP_CLOSE: handle_close(w, conn);
					default: 
						fprintf(stderr, "UNKNOWN OP=%d res=%d\n", op, res);
						break;
				}
			}
			count++;
		}
		io_uring_cq_advance(&w->ring, count);
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


