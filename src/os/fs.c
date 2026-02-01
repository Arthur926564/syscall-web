#include "os/fs.h"
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/fcntl.h>


int os_open(const char *path, int flags) {
	int fd = open(path, flags, 0644);
	if (fd < 0) {
		perror("os_open");
	}
	return fd;
}
ssize_t os_read(int fd, void *buf, size_t size) {
	ssize_t n = read(fd, buf, size);
	if (n < 0) {
		perror("os_read");
		return 0;
	}
	return n;
}

ssize_t os_write(int fd, const void *buf, size_t size) {
	ssize_t n = write(fd, buf, size);
	if (n < 0) {
		perror("os_write");
		return 0;
	}
	return n;
}

int os_close(int fd) {
	if (close(fd) < 0) {
		perror("os_close");
		return -1;
	}
	return 0;
}

int os_set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		perror("fcntl F_GETFL");
		return -1;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		perror("fcntl, F_SETFL | O_NONBLOCK");
		return -1;
	}
	return 0;
}
