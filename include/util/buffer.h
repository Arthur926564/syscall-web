#ifndef UTIL_BUFFER_H
#define UTIL_BUFFER_H

#include <stddef.h>

typedef struct {
	char *data;
	size_t cap;
	size_t start;
	size_t end;
} buffer_t;


void buffer_init(buffer_t *b);
void buffer_free(buffer_t *b);

void buffer_append(buffer_t *b, const void *data, size_t n);
void buffer_consume(buffer_t *b, size_t n);

char *buffer_data(buffer_t *b);
size_t buffer_len(buffer_t *b);

char *write_ptr(buffer_t * b);
size_t write_avail(buffer_t *b);

char *read_ptr(buffer_t *b);


/**
 * tells the buffer that it has succesfuly wrote n bytes into the write_ptr region
 */
void produce(buffer_t *b, size_t n);

int buffer_ensure_writable(buffer_t *b, size_t min_free);

#endif
