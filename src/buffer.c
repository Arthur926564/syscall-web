#include "util/buffer.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void buffer_init(buffer_t *b) {
	if (!b) return;
	
	b->data = NULL;
	b->cap = 0;
	b->len = 0;

}

void buffer_free(buffer_t *b) {
	if (!b) return;
	
	free(b->data);
	free(b);
}

void buffer_append(buffer_t *b, const void *data, size_t n) {
	if (!b) {
		perror("empy buffer");
		return;
	}
	// In case we need to free up some space
	if (b->cap < n + b->len) {
		size_t new_cap = b->cap ? b->cap : 16;
		while (new_cap < b->len + n) {
			new_cap *= 2;
		}

		char *new_data = realloc(b->data, new_cap);
		if (!new_data) {
			perror("realloc failed");
			return;
		}

		b->data = new_data;
		b->cap = new_cap;
	}
	memcpy(b->data + b->len, data, n);
	b->len += n;
}

void buffer_consume(buffer_t *b, size_t n) {
	if (!b || n == 0) return ;

	if (n >= b->len) {
		b->len = 0;
		return;
	}
	memmove(b->data, b->data + n, b->len - n);
}

char *buffer_data(buffer_t *b) {
	return b ? b->data : NULL;
}

size_t buffer_len(buffer_t *b) {
	return b ? b->len : 0;
}


















