#include "http/http_response.h"
#include "core/connection.h"
#include "util/buffer.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>


// I am not really sure about this function, maybe just write it as a classical write ok
void http_response_write_404(buffer_t *out) {
	const char *response_404 =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "\r\n"
    "<!DOCTYPE html>"
    "<html>"
    "<head><title>404 Not Found</title></head>"
    "<body>"
    "<h1>404 Not Found</h1>"
    "<p>The page you requested does not exist.</p>"
    "</body>"
    "</html>";
	buffer_append(out, response_404, strlen(response_404));
}


void http_response_write_file(buffer_t *out,
                              long size,
                              const char *content_type,
                              connection_t *conn) {
    char header[512];
    int header_len;

    header_len = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: %ld\r\n"
        "Content-Type: %s\r\n"
        "Connection: %s\r\n"
        "\r\n",
        size,
        content_type,
        conn->keep_alive ? "keep-alive" : "close"
    );

    if (header_len < 0 || header_len >= (int)sizeof(header)) {
        conn->state = CONN_CLOSED;
        return;
    }

    buffer_append(out, header, (size_t)header_len);
}
