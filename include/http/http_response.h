#include "util/buffer.h"



void http_response_write_file(buffer_t *out, char *data, long size, const char *content_type);

void http_response_write_html(buffer_t *out, const char *html);


void http_response_write_ok(buffer_t *out, char *response);


void http_response_write_404(buffer_t *out);
