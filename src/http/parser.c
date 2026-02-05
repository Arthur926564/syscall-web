#include "http/parser.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>


int http_parse_request(buffer_t *in, http_request_t *req) {
	char *data = in->data;
	size_t len = in->len;

	printf("we are parsing the request\n");
	for (size_t i = 0; i + 1 < len; i++) {
		if (data[i] == '\r' && data[i+1] == '\n') {
			char saved = data[i];
			data[i] = '\0';
			int n = sscanf(data, "%7s %255s %15s",
					req->metod,
					req->path,
					req->version);
			data[i] = saved;
				
			if (n != 3) {
				return -1;
			}
			return (int)(i + 2);
		}
	}
	return 0;

}
