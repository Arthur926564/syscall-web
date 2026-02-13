
#include "http/handler.h"
#include "util/buffer.h"
#include "core/connection.h"
#include "static/static.h"
#include "http/parser.h"
#include "http/http_response.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void handle_request(http_request_t *req, connection_t *conn) {
	if (is_static_request(req)) {
		static_serve(req, conn);
		return;
	} else {
		http_response_write_404(&conn->out);
	}
}

void handle_read(int epfd, connection_t *conn) {
    char tmp[4096];

    while (1) {
        ssize_t n = read(conn->fd, tmp, sizeof(tmp));

        if (n > 0) {
            buffer_append(&conn->in, tmp, (size_t)n);
        }
        else if (n == 0) {
            // client closed connection
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
        ev.events = EPOLLOUT;
        ev.data.ptr = conn;

        epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev);
        conn->state = CONN_WRITING;
        return;
    }

    if (consumed == 0) {
        return;
    }

    // Request complete

	
    buffer_consume(&conn->in, consumed);

    if (is_static_request(&conn->req)) {
        static_serve(&conn->req, conn);
    } else {
        http_response_write_404(&conn->out);
    }

    conn->write_offset = 0;

    struct epoll_event ev;
    ev.events = EPOLLOUT;
    ev.data.ptr = conn;

    epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev);
    conn->state = CONN_WRITING;
}


void handle_write(int epfd, connection_t *conn) {

    while (conn->write_offset < conn->out.len) {

        ssize_t n = write(
            conn->fd,
            conn->out.data + conn->write_offset,
            conn->out.len - conn->write_offset
        );
		printf("[handle_write] fd=%d write_offset=%zu out_len=%zu wrote=%zd\n",conn->fd, conn->write_offset, conn->out.len, n);

        if (n > 0) {
            conn->write_offset += (size_t)n;
        }
        else if (n < 0) {
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


    buffer_init(&conn->out);
    conn->write_offset = 0;

    if (conn->keep_alive) {
        http_request_reset(&conn->req);
        conn->state = CONN_READING_HEADERS;

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.ptr = conn;

        epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev);
    }
    else {
        conn->state = CONN_CLOSED;
		shutdown(conn->fd, SHUT_WR);
    }
}
