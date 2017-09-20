#ifndef YAMBLER_BUFFER_H
#define YAMBLER_BUFFER_H

#include "yambler_type.h"

#include <stddef.h>

struct yambler_byte_buffer{
	yambler_byte *data;
	size_t size;
	size_t length;
};

yambler_status yambler_byte_buffer_create(struct yambler_byte_buffer *buffer, size_t size);

void yambler_byte_buffer_create_unready(struct yambler_byte_buffer *buffer);

yambler_status yambler_byte_buffer_put_all(struct yambler_byte_buffer *buffer, const yambler_byte *in, size_t length);

int yambler_byte_buffer_ready(const struct yambler_byte_buffer *buffer);

void yambler_byte_buffer_destroy(struct yambler_byte_buffer *buffer);

enum yambler_buffer_mode{
	YAMBLER_BUFFER_READ, YAMBLER_BUFFER_WRITE
};

struct yambler_char_buffer{
	yambler_char *data;
	size_t size;
	size_t length;
	yambler_char *get;
	yambler_char *put;
};

yambler_status yambler_char_buffer_create(struct yambler_char_buffer *buffer, size_t initial_size);

yambler_status yambler_char_buffer_grow(struct yambler_char_buffer *buffer, size_t min_capacity);

void yambler_char_buffer_shift(struct yambler_char_buffer *buffer);

void yambler_char_buffer_destroy(struct yambler_char_buffer *buffer);

#endif
