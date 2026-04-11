#include "net/tcp.h"
#include "core/server.h"
#include <stdio.h>
#include <unistd.h>

#define SERVER_PORT 8080

int main(void) {
    // static cache init
    if (static_cache_init(&g_static_cache, "./www") < 0) {
        fprintf(stderr, "failed to initialize static cache\n");
        return 1;
    }
    server_loop(SERVER_PORT); 
    return 0;
}

