#include "net/tcp.h"
#include "core/server.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(void) {
	int server_fd = server_init(8080);
	server_loop(server_fd);
	server_shutdown();
	return 0;
}

