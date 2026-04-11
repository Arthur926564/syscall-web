#include "static/static.h"
#include <dirent.h>
#include <stdbool.h>
#include "http/http_response.h"
#include <magic.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define WWW_ROOT "./www"
#define MAX_PATH 512


static_file_cache_t g_static_cache;

void static_serve(http_request_t *req, connection_t *conn) {
	const static_file_entry_t *entry = static_cache_lookup(&g_static_cache, req->path);

    if (!entry) {
        conn->sending_file = false;
        http_response_write_404(&conn->out);
        return;
    }


	conn->file_fd = entry->fd;
    conn->file_offset = 0;
    conn->file_size = entry->size;
    conn->sending_file = true;

    http_response_write_file(&conn->out, entry->size, entry->content_type, conn);
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



static int scan_dir(static_file_cache_t *cache, const char *root, const char *current);
const char *get_content_type(const char *path); // your existing function

int static_cache_init(static_file_cache_t *cache, const char *root_dir) {
    if (!cache || !root_dir) return -1;
    cache->count = 0;
    return scan_dir(cache, root_dir, root_dir);
}

static int scan_dir(static_file_cache_t *cache, const char *root, const char *current) {
    DIR *dir = opendir(current);
    if (!dir) {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char fullpath[MAX_FILEPATH_LEN];
        int n = snprintf(fullpath, sizeof(fullpath), "%s/%s", current, entry->d_name);
        if (n < 0 || (size_t)n >= sizeof(fullpath)) {
            continue;
        }

        struct stat st;
        if (stat(fullpath, &st) < 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            scan_dir(cache, root, fullpath);
            continue;
        }

        if (!S_ISREG(st.st_mode)) {
            continue;
        }

        if (cache->count >= MAX_STATIC_FILES) {
            fprintf(stderr, "static cache full\n");
            closedir(dir);
            return -1;
        }

        static_file_entry_t *e = &cache->entries[cache->count++];

        memset(e, 0, sizeof(*e));
        strncpy(e->path, fullpath, sizeof(e->path) - 1);

        const char *relative = fullpath + strlen(root);
        if (*relative == '\0') {
            strncpy(e->url, "/", sizeof(e->url) - 1);
        } else {
            strncpy(e->url, relative, sizeof(e->url) - 1);
        }

        e->size = st.st_size;
        e->content_type = get_content_type(fullpath);

        e->fd = open(fullpath, O_RDONLY | O_CLOEXEC);
        if (e->fd < 0) {
            perror("open cached file");
            cache->count--;
            continue;
        }

        if (strcmp(e->url, "/index.html") == 0) {
            // support "/" lookup too if you want
        }
    }

    closedir(dir);
    return 0;
}

const static_file_entry_t *static_cache_lookup(static_file_cache_t *cache, const char *url) {
    if (!cache || !url) return NULL;

    if (strcmp(url, "/") == 0) {
        url = "/index.html";
    }

    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->entries[i].url, url) == 0) {
            return &cache->entries[i];
        }
    }

    return NULL;
}

void static_cache_destroy(static_file_cache_t *cache) {
    if (!cache) return;

    for (size_t i = 0; i < cache->count; i++) {
        if (cache->entries[i].fd >= 0) {
            close(cache->entries[i].fd);
            cache->entries[i].fd = -1;
        }
    }

    cache->count = 0;
}
