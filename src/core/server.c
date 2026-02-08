#include "core/server.h"
#include "util/buffer.h"
#include "core/connection.h"
#include "http/parser.h"
#include "http/handler.h"
#include "net/tcp.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
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
    while (1) {
        int client_fd = tcp_accept(server_fd);
        if (client_fd < 0) continue;

        connection_t *conn = connection_create(client_fd);

        while (1) {
            char tmp[4096];
            ssize_t n = read(conn->fd, tmp, sizeof(tmp));

            if (n < 0) {
                perror("read error in server loop");
                break;
            }
            if (n == 0) {
                break;
            }

            buffer_append(&conn->in, tmp, (size_t)n);

            http_request_t req;
			conn->req = req;
            while (1) {
				http_request_reset(&req);
                int consumed = http_parse_request(&conn->in, &req);
				conn->state = CONN_READING_BODY;


                if (consumed > 0) {
                    handle_request(&req, conn);
                    buffer_consume(&conn->in, consumed);

					while (conn->out.len > 0) {
						ssize_t n = write(conn->fd, conn->out.data, conn->out.len);
						if (n < 0) {
							perror("write error");
							break;
						}
						buffer_consume(&conn->out, n);
					}
					const char *conn_header = get_header(&req, "Connection");
					int keep_alive = conn_header && strcmp(conn_header, "keep_alive");

					http_request_reset(&req);

					if (!keep_alive) {
						conn->state = CONN_READING_HEADERS;
						break;
					}
                } else if (consumed == 0) {
                    // need more data, wait for next read
                    break;
                } else {
                    printf("malformed request\n");
                    break;
                }
            }
        }
        connection_destroy(conn);
    }
}

void server_shutdown() {
	printf("this server is shutting down \n");
}
