
#include "http/handler.h"
#include "util/buffer.h"
#include <stdio.h>
#include <string.h>

void handle_request(http_request_t *req, connection_t *conn) {
	if (strcmp(req->path,"/") == 0) {
		http_response_write_ok(&conn->out, "Hello World");
	} else {
		http_response_write_404(&conn->out);
	}
}

void http_response_write_ok(buffer_t *out, char *response) {
	char header[512];
	int body_len = strlen(response);
	int n = snprintf(header, sizeof(header), 
			        "HTTP/1.1 200 OK\r\n"
                     "Content-Length: %d\r\n"
                     "Content-Type: text/plain\r\n"
                     "\r\n", body_len);

	buffer_append(out, header, n);
	buffer_append(out, response, strlen(response));
}

void http_response_write_404(buffer_t *out) {
	const char *res404 = "HTTP/1.1 404 Not Found\r\n"
                     "Content-Length: 9\r\n"
                     "Content-Type: text/plain\r\n"
                     "\r\n"
                     "Not Found";
	buffer_append(out, res404, strlen(res404));
}
