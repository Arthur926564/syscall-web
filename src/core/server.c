#include "core/server.h"
#include "os/fs.h"
#include "util/buffer.h"
#include "core/connection.h"
#include "http/parser.h"
#include "http/handler.h"
#include "net/tcp.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>



int server_init(int port) {
	int server_fd = tcp_listen(port);
	if (server_fd < 0) {
		perror("tcp_accept error in server init");
		return -1;
	}
	printf("Listening on port 8080\n");
	return server_fd;

}

void server_loop(int server_fd) {
	// create the epoll
	int epfd = epoll_create(1);
	if (epfd == -1) {
		perror("epoll_create");
	}
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.ptr = NULL;

	int epoll_ctl_res = epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

	struct epoll_event events[32];
	while (1) {
		
		int n = epoll_wait(epfd, events, 32, -1);
		if (n == -1) {
			if (errno == EINTR) {
				continue;
			}
			perror("epoll wait");
			exit(1);
		}

		for (size_t i = 0; i < n; i++) {
			struct epoll_event *ev = &events[i];

			if (ev->data.ptr == NULL) {
				struct sockaddr client_addr;
				socklen_t addrlen = sizeof(client_addr);
				int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
				if (client_fd == -1) {
					perror("accept");
					continue;
				}
				os_set_nonblocking(client_fd);
				connection_t *conn = malloc(sizeof(connection_t));
				conn->fd = client_fd;
				buffer_init(&conn->in);
				buffer_init(&conn->out);

				conn->state = CONN_READING_HEADERS;

				struct epoll_event cev;
				cev.events = EPOLLIN | EPOLLET;
				cev.data.ptr = conn;
				if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev) == -1) {
					perror("epoll_ctl client_fd");
					close(client_fd);
					free(conn);
				}

			} else {
				connection_t *conn = ev->data.ptr;

				if (ev->events & EPOLLIN) {
					handle_read(epfd, conn);
				}
				
				if (ev->events & EPOLLOUT) {
					handle_write(epfd, conn);
				}

				if (conn->state == CONN_CLOSED) {
					epoll_ctl(epfd, EPOLL_CTL_DEL, conn->fd, NULL);
					free(conn->in.data);
					free(conn->out.data);
					free(conn);
				}

			}

		}
	}
	
}



void server_shutdown() {
	printf("this server is shutting down \n");
}
