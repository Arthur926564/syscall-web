

#include <stdint.h>

typedef union epoll_data {
	void *ptr;
	int fd;
	uint32_t u32;
	uint64_t u64;
} epoll_data;

struct epoll_event{
	uint32_t events;
	epoll_data_t data;
};
