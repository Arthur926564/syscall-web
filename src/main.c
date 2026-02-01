#include "os/fs.h"
#include "os/signals.h"
#include "os/time.h"
#include <fcntl.h>
#include <stdio.h>

int main() {
	os_setup_signals();

    int fd = os_open("www/index.html", O_RDONLY);
    if (fd < 0) return 1;

    char buf[128];
    ssize_t n = os_read(fd, buf, sizeof(buf)-1);
    if (n > 0) {
        buf[n] = '\0';
        printf("Read %zd bytes:\n%s\n", n, buf);
    }
    os_close(fd);

    int timer_fd = os_create_timer(1000);
    for (int i = 0; i < 5; i++) {
        os_wait_timer(timer_fd);
        printf("Timer tick %d\n", i+1);
    }

    return 0;
}

