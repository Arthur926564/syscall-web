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
        if (client_fd < 0)
            continue;

        connection_t *conn = connection_create(client_fd);

        http_request_t *req = &conn->req;
        http_request_reset(req);
        conn->state = CONN_READING_HEADERS;

        while (conn->state != CONN_CLOSED) {

            switch (conn->state) {

            case CONN_READING_HEADERS: {
                char tmp[4096];
                ssize_t n = read(conn->fd, tmp, sizeof(tmp));

                if (n <= 0) {
                    conn->state = CONN_CLOSED;
                    break;
                }

                buffer_append(&conn->in, tmp, (size_t)n);

                int consumed = http_parse_request(&conn->in, req);

                if (consumed < 0) {
                    // malformed request
                    conn->state = CONN_CLOSED;
                } else if (consumed == 0) {
                    // need more data → stay in this state
                } else {
                    // headers complete
                    buffer_consume(&conn->in, consumed);

                    if (consumed == 3)
                        conn->state = CONN_READING_BODY;
                    else
                        conn->state = CONN_WRITING;
                }
                break;
            }

            case CONN_READING_BODY:
                conn->state = CONN_WRITING;
                break;

            case CONN_WRITING:
                handle_request(req, conn);

                write(conn->fd, conn->out.data, conn->out.len);
				buffer_init(&conn->out);

				const char* connection_header = get_header(req, "Connection");
				
                if (connection_header && !strcmp(connection_header, "keep-alive")) {
					printf("is this correct or not?\n");
                    http_request_reset(req);
                    conn->state = CONN_READING_HEADERS;
                } else {
                    conn->state = CONN_CLOSED;
                }
                break;

            default:
                conn->state = CONN_CLOSED;
                break;
            }
        }

        connection_destroy(conn);
    }
}



void server_shutdown() {
	printf("this server is shutting down \n");
}
