#include "os/signals.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>



void os_signal_handler(int signum) {
	printf("Signal received %d\n", signum);
	// TODO close sockets/ freee memory
	exit(0);
}

void os_setup_signals() {
	signal(SIGINT, os_signal_handler);
	signal(SIGTERM, os_signal_handler);
}
