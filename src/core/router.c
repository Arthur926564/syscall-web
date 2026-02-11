#include "core/router.h"
#include <stdbool.h>
#include "http/parser.h"
#include "http/http_response.h"
#include "static/static.h"
#include <stdio.h>
#include <string.h>


void router_handle(http_request_t *req, connection_t *conn) {
	if (is_static_request(req)) {
		static_serve(req, conn);
	} else if (strcmp(req->path, "api/") == 0) {
		printf("this is not implemented at the moment ......\n");
	} else {
		http_response_write_404(&conn->out);
	}
}
