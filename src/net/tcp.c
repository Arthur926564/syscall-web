#include "net/tcp.h"

#include <sys/socket.h>
#include <linux/socket.h>
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

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		perror("setsockopt");
		close(fd);
		return -1;
	}

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


int tcp_listen_reuseport(int port) {
	int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (fd < 0) {
		return -1;
	}
	int one = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = INADDR_ANY,
	};

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(fd);
		return -1;
	}
	if (listen(fd, 511) < 0) {
		close(fd);
		return -1;
	}
	return fd;
	
}

int tcp_accept(int server_fd) {
	return accept(server_fd, NULL, NULL);
}

