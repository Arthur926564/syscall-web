#include "http/parser.h"
#include "core/connection.h"

void static_serve(http_request_t *req, connection_t *conn);

const char* get_content_type(const char *path);

int resolve_path(const char *url, char *out_path);
