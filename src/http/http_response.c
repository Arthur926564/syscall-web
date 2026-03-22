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


static int u64toa(char *buf, long v) {
	if (v == 0) {
		buf[0] = '0'; return  1;
	}
	char tmp[20];
	int i = 0;
	while (v > 0) {
		tmp[i++] = '0' + (v % 10);
		v /= 10;
	}
	for (int j = 0; j  < i; j++) {
		buf[j] = tmp[i - 1- j];
	}
	return i;
}


void http_response_write_file(buffer_t *out,
                              long size,
                              const char *content_type,
                              connection_t *conn) {
	static const char prefix[] = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: ";

	static const char ka_ct[] = "\r\nConnection: keep-alive\r\n\r\n";
	static const char cl_ct[] = "\r\nConnection:  close\r\n\r\n";

	char len_str[20];
	int len_digits = u64toa(len_str, size);

	static const char ct_prefix[] = "\r\nContent-Type: ";


	const char* suffix = conn->keep_alive ? ka_ct : cl_ct;
	size_t suffix_len = conn->keep_alive ?  sizeof(ka_ct) - 1: sizeof(cl_ct) - 1;

	buffer_append(out, prefix, sizeof(prefix) - 1);
	buffer_append(out, len_str, len_digits);
	buffer_append(out, ct_prefix, sizeof(ct_prefix) - 1);
	buffer_append(out, content_type, strlen(content_type));
	buffer_append(out, suffix, suffix_len);
}
