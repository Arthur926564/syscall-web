#include "net/tcp.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(void) {
	printf("yes or no \n");
    int server_fd = tcp_listen(8080);
    if (server_fd < 0)
        return 1;

    printf("Listening on port 8080...\n");

    while (1) {
        int client_fd = tcp_accept(server_fd);
        if (client_fd < 0)
            continue;
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

