#include "core/server.h"
#include "core/worker.h"
#include "net/tcp.h"
#include "os/fs.h"

#include <errno.h>
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

	printf("Listening on port %d\n", port);
	return server_fd;
}

static void accept_and_dispatch(int server_fd, worker_t *workers, int nworkers) {
	int next_worker = 0;

	while (1) {
		struct sockaddr client_addr;
		socklen_t addrlen = sizeof(client_addr);

		int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
		if (client_fd == -1) {
			if (errno == EINTR) {
				continue;
			}
			perror("accept");
			continue;
		}

		os_set_nonblocking(client_fd);

		if (worker_enqueue_client(&workers[next_worker], client_fd) == -1) {
			close(client_fd);
		}

		next_worker = (next_worker + 1) % nworkers;
	}
}

void server_loop(int server_fd) {
	worker_t workers[NWORKERS];

	for (int i = 0; i < NWORKERS; i++) {
		if (worker_init(&workers[i]) == -1) {
			fprintf(stderr, "worker_init failed for worker %d\n", i);
			close(server_fd);
			exit(1);
		}

		if (worker_start(&workers[i]) == -1) {
			fprintf(stderr, "worker_start failed for worker %d\n", i);
			close(server_fd);
			exit(1);
		}
	}

	accept_and_dispatch(server_fd, workers, NWORKERS);
}

void server_shutdown(int server_fd) {
	os_close(server_fd);
	printf("this server is shutting down\n");
}
