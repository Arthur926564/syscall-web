#include "util/buffer.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void buffer_init(buffer_t *b) {
	if (!b) return;
	b->data = NULL;
	b->cap = 0;
	b->start = 0;
	b->end = 0;
}

void buffer_free(buffer_t *b) {
	if (!b) return;
	
	free(b->data);
}

void buffer_append(buffer_t *b, const void *data, size_t n) {
	if (!b) {
		perror("empty buffer");
		return;
	}
	// In case we need to free up some space
	if (b->cap < n + b->end) {
		if (b->start > 0 && n + buffer_len(b) <= b->cap)  {
			size_t old_len = buffer_len(b);
			memmove(b->data, b->data + b->start, old_len);
			b->start = 0;
			b->end = old_len;
			
		} else {
			size_t new_cap = b->cap ? b->cap : 16;
			while (new_cap < b->end + n) {
				new_cap *= 2;
			}
			b->cap = new_cap;
			
			char *new_data = realloc(b->data, b->cap);
			if (!new_data) {
				perror("realloc failed");
				return;
			}

			b->data = new_data;
		}
	}

	memcpy(b->data + b->end, data, n);
	b->end += n;
}

void buffer_consume(buffer_t *b, size_t n) {
	if (!b || n == 0) return ;
	b->start += n;
}

char *buffer_data(buffer_t *b) {
	return b ? b->data + b->start : NULL;
}

size_t buffer_len(buffer_t *b) {
	return b ? b->end - b->start : 0;
}


















