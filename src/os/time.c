#include "os/time.h"
#include <bits/types/struct_itimerspec.h>
#include <stdint.h>
#include <sys/timerfd.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>



int os_create_timer(int interval_ms) {
	int fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (fd < 0) {
		perror("timerfd_create");
		return -1;
	}
	struct itimerspec ts;
	ts.it_interval.tv_sec = interval_ms / 1000;
	ts.it_interval.tv_nsec = (interval_ms % 1000) * 1000000;
	ts.it_value = ts.it_interval;

	if (timerfd_settime(fd, 0, &ts, NULL) < 0) {
		perror("timerfd_settime");
		close(fd);
		return -1;
	}
	return fd;
}

int os_wait_timer(int timer_fd) {
	uint64_t expirations;
	ssize_t s = read(timer_fd, &expirations, sizeof(expirations));
	if (s != sizeof(expirations)) {
		perror("os_wait_timer read");
		return -1;
	}
	return (int)expirations;
}
