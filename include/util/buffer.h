#ifndef UTIL_BUFFER_H
#define UTIL_BUFFER_H

#include <stddef.h>

typedef struct {
	char *data;
	size_t len;
	size_t cap;
} buffer_t;


void buffer_init(buffer_t *b);
void buffer_free(buffer_t *b);

void buffer_append(buffer_t *b, const void *data, size_t n);
void buffer_consume(buffer_t *b, size_t n);

char *buffer_data(buffer_t *b);
size_t buffer_len(buffer_t *b);

#endif
