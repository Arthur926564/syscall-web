#include "net/tcp.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>



int tcp_listen(uint16_t port) {
	int fd = socket(AF_INET,SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
			return -1;
	}
	int yes = 1;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(port)
	};

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(fd);
		return -1;
	}
	if (listen(fd,128) < 0) {
		perror("listern");
		close(fd);
		return -1;
	}
	return fd;
}

int tcp_accept(int server_fd) {
	return accept(server_fd, NULL, NULL);
}

