#include "http/parser.h"
#include "core/connection.h"


void handle_request(http_request_t *req, connection_t *conn);


void http_response_write_ok(buffer_t *out, char *response);


void http_response_write_404(buffer_t *out);
