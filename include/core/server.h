

#include <sys/epoll.h>
#include "static/static.h"


extern static_file_cache_t g_static_cache;

int server_init(int port);

void server_loop(int server_fd);

void server_shutdown(int server_fd);


static void accept_new_clients(int epfd, int server_fd);

