#include <stdlib.h>
#define WWW_ROOT "./www"
#define MAX_PATH 512

#include "static/static.h"
#include "http/http_response.h"
#include <magic.h>
#include <stdio.h>
#include <string.h>


void static_serve(http_request_t *req, connection_t *conn) {
	char filepath[MAX_PATH];

	if (resolve_path(req->path, filepath) < 0) {
		http_response_write_404(&conn->out);
		return;
	}
	FILE *f = fopen(filepath, "rb");
	if (!f) {
		http_response_write_404(&conn->out);
		return;
	}


	fseek(f, 0, SEEK_END);
	long size = ftell(f);

	fseek(f, 0, SEEK_SET);


	char *data = malloc(size);
	fread(data, 1,size, f);

	const char *content_type = get_content_type(filepath);

	http_response_write_file(&conn->out, data, size, content_type);

	free(data);
}

const char* get_content_type(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";  
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    return "application/octet-stream";
}

int resolve_path(const char *url, char *out_path) {
	if (!url || !out_path) return -1;

	if (strstr(url, "..")) {
		return -1;
	}

	if (strcmp(url, "/") == 0) {
		snprintf(out_path, MAX_PATH,   "%s/index.html", WWW_ROOT);
		return 0;
	}


	if (url[0] != '/') {
		return -1;
	}


	snprintf(out_path, MAX_PATH, "%s%s", WWW_ROOT, url);
	return 0;
	
}
