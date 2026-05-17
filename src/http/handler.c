
#include <dirent.h>    // DIR, opendir, readdir, closedir
#include <sys/resource.h>  // setrlimit, getrlimit, struct rlimit
#include "http/handler.h"
#include "core/conn_pool.h"
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include "io/io_ops.h"
#include "io/io_uring.h"
#include <unistd.h>

#define MAX_KEEP_CAP (64 * 1024)

static inline void conn_rearm(int epfd, connection_t *conn, uint32_t events) {
	struct epoll_event ev = {
		.events = events | EPOLLET | EPOLLONESHOT | EPOLLRDHUP,
		.data.ptr = conn,
	};
	epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev);
}


void handle_request(http_request_t *req, connection_t *conn) {
	if (is_static_request(req)) {
		static_serve(req, conn);
		return;
	} else {
		http_response_write_404(&conn->out);
	}
}

void handle_read(int epfd, connection_t *conn) {
	int avail = buffer_ensure_writable(&conn->in, 8192);

	if (avail < 0) {
		perror("available error");
		conn->state = CONN_CLOSED;
		return;
	}

    while (1) {
		char * ptr = write_ptr(&conn->in);

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

	conn_rearm(epfd, conn, EPOLLOUT);
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
					struct epoll_event ev;
        			ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
					ev.data.ptr = conn;
					epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev);
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

		if (conn->file_offset >= conn->file_size) {
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
		conn_rearm(epfd, conn, EPOLLIN);

    }
    else {
        conn->state = CONN_CLOSED;
		shutdown(conn->fd, SHUT_WR);
    }
}


// =================== new ========================
//
//
void handle_accept(worker_t *w, int client_fd, uint32_t flags) {
	DIR *fd_dir = opendir("/proc/self/fd");
    int count = 0;
    while (readdir(fd_dir)) count++;
    closedir(fd_dir);
    if (!(flags & IORING_CQE_F_MORE)) {
        io_add_accept(w);
    }
    if (client_fd < 0){
		return;  
	} 

    connection_t *conn = conn_pool_get(&w->pool);
    if (!conn) { close(client_fd); return; }

    conn->fd    = client_fd;
    conn->state = CONN_READING_HEADERS;
    
    // ensure buffer has space then submit recv
    buffer_ensure_writable(&conn->in, 8192);
    io_add_recv(w, conn);
    io_uring_submit(&w->ring);
}


void handle_recv(worker_t *w, connection_t *conn, int res) {
    conn->inflight--;
    if (conn->closing) { conn_maybe_close(w, conn); return; }
    if (res <= 0) { io_add_close(w, conn); return; }

    produce(&conn->in, res);

    // check if buffer has a complete request
    int consumed = http_parse_request(&conn->in, &conn->req);
    if (consumed == 0) {
        // incomplete — read more, still in recv state
        buffer_ensure_writable(&conn->in, 8192);
        io_add_recv(w, conn);
        return;
    }
    if (consumed < 0) {
        http_response_write_404(&conn->out);
        conn->keep_alive = false;
        conn->state = CONN_WRITING;  // ← transition state
        io_add_send(w, conn);
        return;
    }

    conn->keep_alive = keep_alive(&conn->req);
    buffer_consume(&conn->in, consumed);

    if (is_static_request(&conn->req)) {
        static_serve(&conn->req, conn);
    } else {
        http_response_write_404(&conn->out);
    }

    conn->state = CONN_WRITING;  // ← transition to writing, no more recvs
    io_add_send(w, conn);
}



void handle_send(worker_t *w, connection_t *conn, int res) {
    conn->inflight--;  // ← add this if missing
    if (conn->closing) { conn_maybe_close(w, conn); return; }
    
    
    if (res <= 0) { io_add_close(w, conn); return; }
    conn->write_offset += res;

    if (conn->write_offset < buffer_len(&conn->out)) {
        io_add_send(w, conn);
        return;
    }

    buffer_reset_and_maybe_shrink(&conn->out, MAX_KEEP_CAP);
    conn->write_offset = 0;

    if (conn->sending_file) {
        while (conn->file_offset < conn->file_size) {
            ssize_t n = sendfile(conn->fd, conn->file_fd,
                                 &conn->file_offset,
                                 conn->file_size - conn->file_offset);


            if (n > 0) continue;
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                io_add_send(w, conn);  // wait for socket ready
                return;
            }
            if (n <= 0) { 
				io_add_close(w, conn);
				return; 
			}
        }
        conn->file_fd      = -1;
        conn->file_offset  = 0;
        conn->file_size    = 0;
        conn->sending_file = false;
    }

    if (conn->keep_alive) {
        http_request_reset(&conn->req);
        buffer_reset_and_maybe_shrink(&conn->in, MAX_KEEP_CAP);
        conn->write_offset = 0;
        conn->state = CONN_READING_HEADERS;  // ← back to reading
        buffer_ensure_writable(&conn->in, 8192);
        io_add_recv(w, conn);  // ← only NOW submit next recv
    } else {
        io_add_close(w, conn);
    }
}



void handle_sendfile(worker_t *w, connection_t *conn, int res) {
    if (res <= 0) {
        io_add_close(w, conn);
        return;
    }
    conn->file_offset += res;
	conn->inflight--;
	if (conn->closing) {
		conn_maybe_close(w, conn);
		return;
	}

    if (conn->file_offset < conn->file_size) {
        // partial splice — continue
        io_add_sendfile(w, conn);
        return;
    }

    // file fully sent
    conn->file_fd      = -1;
    conn->file_offset  = 0;
    conn->file_size    = 0;
    conn->sending_file = false;

    if (conn->keep_alive) {
        http_request_reset(&conn->req);
		buffer_reset_and_maybe_shrink(&conn->in, MAX_KEEP_CAP);
		buffer_reset_and_maybe_shrink(&conn->out, MAX_KEEP_CAP);
		conn->write_offset = 0;
        conn->state = CONN_READING_HEADERS;
        buffer_ensure_writable(&conn->in, 8192);
        io_add_recv(w, conn);
    } else {
        io_add_close(w, conn);
    }
}

void handle_close(worker_t *w, connection_t *conn) {
	buffer_reset_and_maybe_shrink(&conn->in, MAX_KEEP_CAP);
	buffer_reset_and_maybe_shrink(&conn->out, MAX_KEEP_CAP);
	close(conn->fd);
	conn->fd = -1;
    conn_pool_put(&w->pool, conn);  
}
