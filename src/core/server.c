#include "core/server.h"
#include "net/tcp.h"
#include <stdio.h>
#include <string.h>
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
		// This is something chatgpt gave me but I am kind of lost with all of this at this point...
		const char *response =
    		"HTTP/1.1 200 OK\r\n"
    		"Content-Length: 20\r\n"
    		"Content-Type: text/plain\r\n"
    		"Connection: close\r\n"
    		"\r\n"
    		"Hello from C server\n";
		write(client_fd, response, strlen(response));
        close(client_fd);

	}
}

void server_shutdown() {
	printf("this server is shutting down \n");
}
