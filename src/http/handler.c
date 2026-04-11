
#include "http/handler.h"
#include "util/buffer.h"
#include "core/connection.h"
#include "static/static.h"
#include "http/parser.h"
#include "http/http_response.h"
#include <netinet/in.h>
#include <linux/tcp.h>
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <unistd.h>

#define MAX_KEEP_CAP (64 * 1024)

void handle_request(http_request_t *req, connection_t *conn) {
	if (is_static_request(req)) {
		static_serve(req, conn);
		return;
	} else {
		http_response_write_404(&conn->out);
	}
}

void handle_read(int epfd, connection_t *conn) {

    while (1) {
		int avail = buffer_ensure_writable(&conn->in, 1024);
		char * ptr = write_ptr(&conn->in);
		

		if (avail < 0) {
			perror("available error");
			conn->state = CONN_CLOSED;
			return;
		}

        ssize_t n = read(conn->fd, ptr, avail);

        if (n > 0) {
			produce(&conn->in, n);
        }
        else if (n == 0) {
            conn->state = CONN_CLOSED;
            return;
        }
        else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // no more data available now
                break;
            } else {
                perror("read");
                conn->state = CONN_CLOSED;
                return;
            }
        }
    }

    // Try parsing after reading
    int consumed = http_parse_request(&conn->in, &conn->req);

    if (consumed < 0) {
        http_response_write_404(&conn->out);

        conn->write_offset = 0;

        struct epoll_event ev;
        ev.events = EPOLLOUT | EPOLLET;
        ev.data.ptr = conn;

        epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev);
        conn->state = CONN_WRITING;
        return;
    }

    if (consumed == 0) {
        return;
    }
	conn->keep_alive = keep_alive(&conn->req);

    // Request complete

	
    buffer_consume(&conn->in, consumed);

    if (is_static_request(&conn->req)) {
        static_serve(&conn->req, conn);
    } else {
        http_response_write_404(&conn->out);
    }

    conn->write_offset = 0;

    struct epoll_event ev;
    ev.events = EPOLLOUT | EPOLLET;
    ev.data.ptr = conn;

    epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev);
    conn->state = CONN_WRITING;
}


void handle_write(int epfd, connection_t *conn) {

    while (conn->write_offset < buffer_len(&conn->out)) {

        ssize_t n = write(
            conn->fd,
            conn->out.data + conn->out.start + conn->write_offset,
            buffer_len(&conn->out) - conn->write_offset
        );

        if (n > 0) {
            conn->write_offset += (size_t)n;
        }
        else if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Socket not ready for more writing
                return;
            } else {
                perror("write");
                conn->state = CONN_CLOSED;
                return;
            }
        }
        else {
            // write returned 0 (rare but treat as closed)
            conn->state = CONN_CLOSED;
            return;
        }
    }
	if (conn->write_offset == buffer_len(&conn->out)) {
		buffer_reset_and_maybe_shrink(&conn->out, MAX_KEEP_CAP);
		conn->write_offset = 0;
	}

	if (conn->sending_file) {
		int cork = 1;
		setsockopt(conn->fd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));
		while (conn->file_offset < conn->file_size) {
			ssize_t n = sendfile(
					conn->fd,
					conn->file_fd,
					&conn->file_offset,
					(size_t)(conn->file_size - conn->file_offset)
					);
			if (n > 0) {
				continue;
			} else if (n < 0) {
				if (errno == EINTR) {
					continue;
				}
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					return;
				} else {
					perror("sending_file");
					close(conn->file_fd);
					conn->file_fd = -1;
					conn->sending_file = false;
					conn->state = CONN_CLOSED;
					return;
				}
			} else {
				break;
			}
		}

		cork = 0;
		setsockopt(conn->fd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));


		if (conn->file_offset >= conn->file_size) {
			close(conn->file_fd);
			conn->file_fd = -1;
			conn->file_offset = 0;
			conn->file_size = 0;
			conn->sending_file = false;
		} else {
			return;
		}
	}

    if (conn->keep_alive) {
        http_request_reset(&conn->req);
        conn->state = CONN_READING_HEADERS;

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = conn;

        if (epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev) == -1) {
			perror("epoll_ctl MODE EPOLLIN");
			conn->state = CONN_CLOSED;
			return;
		}
    }
    else {
        conn->state = CONN_CLOSED;
		shutdown(conn->fd, SHUT_WR);
    }
}
