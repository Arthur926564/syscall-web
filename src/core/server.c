#include <pthread.h>
#include <string.h>
#include "core/server.h"
#include "core/worker.h"
#include "net/tcp.h"
#include "os/fs.h"
#include "static/static.h"
#include <dirent.h>    // DIR, opendir, readdir, closedir
#include <sys/resource.h>  // setrlimit, getrlimit, struct rlimit

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define NWORKERS 8

int server_init(int port) {
	int server_fd = tcp_listen(port);
	if (server_fd < 0) {
		perror("tcp_listen error in server_init");
		return -1;
	}

	if (static_cache_init(&g_static_cache, "./www") < 0) {
		fprintf(stderr, "failed to initialize static cache\n");
		exit(1);
	}

	return server_fd;
}


void server_loop(int port) {
	worker_t *workers = calloc(NWORKERS, sizeof(worker_t));

	for (int i = 0; i < NWORKERS; i++) {
		if (worker_init(&workers[i], port) < 0) {
			exit(1);
		}
		if (worker_start(&workers[i]) < 0) {
			exit(1);
		}
		DIR *fd_dir = opendir("/proc/self/fd");
        int count = 0;
        while (readdir(fd_dir)) count++;
        closedir(fd_dir);
	}

	for (int i = 0; i < NWORKERS; i++) {
		pthread_join(workers[i].thread, NULL);
	}

}

void server_shutdown(int server_fd) {
	os_close(server_fd);
	printf("this server is shutting down\n");
	static_cache_destroy(&g_static_cache);
}
