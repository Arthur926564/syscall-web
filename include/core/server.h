

#include <sys/epoll.h>
int server_init(int port);

void server_loop(int server_fd);

void server_shutdown(int server_fd);


static void accept_new_clients(int epfd, int server_fd);

