
#include "http/handler.h"
#include "static/static.h"
#include "http/parser.h"
#include "http/http_response.h"
#include <stdio.h>
#include <string.h>

void handle_request(http_request_t *req, connection_t *conn) {
	printf("static serve called for %s \n", req->path);
	if (is_static_request(req)) {
		printf("I ma going ");
		static_serve(req, conn);
		return;
	} 
	
	if (strcmp(req->path,"/") == 0) {
		const char *html = "<!DOCTYPE html><html><body><h1>Hello Browser!</h1></body></html>";
    	http_response_write_html(&conn->out, html);
	} else {
		http_response_write_404(&conn->out);
	}
}

