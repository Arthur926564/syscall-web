
#include "http/handler.h"
#include "static/static.h"
#include "http/parser.h"
#include "http/http_response.h"

void handle_request(http_request_t *req, connection_t *conn) {
	if (is_static_request(req)) {
		static_serve(req, conn);
		return;
	} else {
		http_response_write_404(&conn->out);
	}
}

