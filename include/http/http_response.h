#include "core/connection.h"
#include "util/buffer.h"



void http_response_write_file(buffer_t *out, char *data, long size, const char *content_type, conn_state_t state);

void http_response_write_html(buffer_t *out, const char *html);


void http_response_write_404(buffer_t *out);
