#include "net/tcp.h"
#include "core/server.h"
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>    // DIR, opendir, readdir, closedir
#include <sys/resource.h>  // setrlimit, getrlimit, struct rlimit

#define SERVER_PORT 8080

int main(void) {
    struct rlimit rl = { 65535, 65535 };
    setrlimit(RLIMIT_NOFILE, &rl);
    
    fprintf(stderr, "before cache init\n");
    // count open fds
    DIR *fd_dir = opendir("/proc/self/fd");
    int count = 0;
    while (readdir(fd_dir)) count++;
    closedir(fd_dir);
    fprintf(stderr, "fds open at start: %d\n", count);

    if (static_cache_init(&g_static_cache, "./www") < 0) {
		fprintf(stderr, "error\n");
		return 0;
	}
    
    fprintf(stderr, "after cache init\n");
    // count again
    fd_dir = opendir("/proc/self/fd");
    count = 0;
    while (readdir(fd_dir)) count++;
    closedir(fd_dir);
    fprintf(stderr, "fds after cache: %d\n", count);

    server_loop(SERVER_PORT);
    return 0;
}

