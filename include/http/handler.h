#include "http/parser.h"
#include "core/connection.h"


void handle_request(http_request_t *req, connection_t *conn);


void handle_read(int epfd, connection_t *conn);

void handle_write(int epfd, connection_t *conn);

