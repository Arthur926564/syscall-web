#include "http/http_response.h"
#include "util/buffer.h"

#include <stdio.h>
#include <string.h>





void http_response_write_html(buffer_t *out, const char *html) {
    char header[512];
    int body_len = strlen(html);
    int n = snprintf(header, sizeof(header),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Length: %d\r\n"
                     "Content-Type: text/html\r\n"
                     "Connection: close\r\n"
                     "\r\n", body_len);
    buffer_append(out, header, n);
    buffer_append(out, html, body_len);
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



void http_response_write_file(buffer_t *out,
							char *data,
							long size,
							const char *content_type) {
	char header[512];
	int header_len = snprintf(
    	header,
    	sizeof(header),
    	"HTTP/1.1 200 OK\r\n"
    	"Content-Length: %ld\r\n"
    	"Content-Type: %s\r\n"
    	"Connection: close\r\n"
    	"\r\n",
    	size,          // first long (%ld)
    	content_type   // second string (%s)
	);



	buffer_append(out, header, header_len);
	buffer_append(out, data, size);
	
}
