#include "core/server.h"
#include "core/connection.h"
#include "net/tcp.h"
#include <stdio.h>
#include <unistd.h>



int server_init(int port) {
	int server_fd = tcp_listen(port);
	if (server_fd < 0) {
		perror("tcp_accept error in server init");
		return -1;
	}
	printf("Listening on porte 8080\n");
	return server_fd;

}

void server_loop(int server_fd) {
	while (1) {
		int client_fd = tcp_accept(server_fd);
		if (client_fd < 0) {
			continue;
		}
		printf("this is another but )ASJASLKFJALKFJAKFJALKSFJASLKFJALK test here\n");
		connection_t *conn = connection_create(client_fd);
		int n = read(conn->fd, &conn->in, 15);
		printf("this is another test here\n");

		if (n > 0) { 
			printf("this is a test here\n");
			printf("this is the value of n %d\n", n);
			// blablabla
		}

		connection_destroy(conn);

	}
}

void server_shutdown() {
	printf("this server is shutting down \n");
}
