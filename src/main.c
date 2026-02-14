#include "net/tcp.h"
#include "core/server.h"
#include <unistd.h>

int main(void) {
	int server_fd = server_init(8080);
	server_loop(server_fd);
	server_shutdown(server_fd);
	return 0;
}

