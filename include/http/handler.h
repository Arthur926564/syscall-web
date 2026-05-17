#include "core/worker.h"
#include "http/parser.h"
#include "core/connection.h"


void handle_request(http_request_t *req, connection_t *conn);

void handle_accept(worker_t *w, int client_fd, uint32_t flags);

void handle_recv(worker_t *w, connection_t *conn, int res);


void handle_send(worker_t *w, connection_t *conn, int res);

void handle_sendfile(worker_t *w, connection_t *conn, int res);

void handle_close(worker_t *w, connection_t *conn);

void handle_read(int epfd, connection_t *conn);

void handle_write(int epfd, connection_t *conn);

