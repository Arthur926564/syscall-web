#include <stdbool.h>
#include <stddef.h>
#include "http/parser.h"
#include "core/connection.h"

#ifndef STATIC_CACHE_H
#define STATIC_CACHE_H


#define WWW_ROOT "./www"
#define MAX_PATH 512
#define MAX_STATIC_FILES 1024
#define MAX_FILEPATH_LEN 512


typedef struct {
	char url[256];
	char path[MAX_PATH];
	off_t size;
	const char *content_type;
	int fd;
} static_file_entry_t;


typedef struct {
	static_file_entry_t entries[MAX_STATIC_FILES];
	size_t count;
} static_file_cache_t;


extern static_file_cache_t g_static_cache;


void static_serve(http_request_t *req, connection_t *conn);

const char* get_content_type(const char *path);

int resolve_path(const char *url, char *out_path);


int static_cache_init(static_file_cache_t *cache, const char *root_dir);
const static_file_entry_t *static_cache_lookup(static_file_cache_t *cache, const char *url);
void static_cache_destroy(static_file_cache_t *cache);

#endif
