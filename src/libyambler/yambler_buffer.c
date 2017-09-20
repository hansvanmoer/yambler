#include "yambler_buffer.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

yambler_status yambler_byte_buffer_create(struct yambler_byte_buffer *buffer, size_t size){
	assert(buffer != NULL);
	assert(size != 0);

	buffer->data = malloc(sizeof(yambler_byte) * size);
	if(buffer->data == NULL){
		return YAMBLER_ALLOC_ERROR;
	}
	buffer->size = size;
	return YAMBLER_OK;
}

void yambler_byte_buffer_create_unready(struct yambler_byte_buffer *buffer){
	assert(buffer != NULL);

	buffer->data = NULL;
	buffer->size = 0;
}

yambler_status yambler_byte_buffer_put_all(struct yambler_byte_buffer *buffer, const yambler_byte *in, size_t length){
	if(buffer->size - buffer->length > length){
		return YAMBLER_FULL;
	}
	memcpy(buffer->data+buffer->length, in, length);
	buffer->length+=length;
	return YAMBLER_OK;
}

int yambler_byte_buffer_ready(const struct yambler_byte_buffer *buffer){
	return buffer->data != NULL;
}

void yambler_byte_buffer_destroy(struct yambler_byte_buffer *buffer){
	assert(buffer != NULL);
	free(buffer->data);
}

yambler_status yambler_char_buffer_create(struct yambler_char_buffer *buffer, size_t initial_size){
	assert(buffer != NULL);
	buffer->data = malloc(sizeof(yambler_char) * initial_size);
	if(buffer->data == NULL){
		return YAMBLER_ALLOC_ERROR;
	}
	buffer->size = initial_size;
	buffer->length = 0;
	buffer->get = buffer->put = buffer->data;
	return YAMBLER_OK;
}

void yambler_char_buffer_shift(struct yambler_char_buffer *buffer){
	memmove(buffer->data, buffer->get, buffer->length * sizeof(yambler_char));
	buffer->put = buffer->get + buffer->length;
}

yambler_status yambler_char_buffer_grow(struct yambler_char_buffer *buffer, size_t min_capacity){
	if(min_capacity > buffer->size){
		yambler_char *data = malloc(sizeof(yambler_char) * min_capacity);
		if(data == NULL){
			return YAMBLER_ALLOC_ERROR;
		}
		memcpy(data, buffer->get, buffer->length);
		free(buffer->data);
		buffer->data = data;
		buffer->get = data;
		buffer->put = data + buffer->length;
		buffer->size = min_capacity;
	}
	return YAMBLER_OK;
}

void yambler_char_buffer_destroy(struct yambler_char_buffer *buffer){
	assert(buffer != NULL);
	free(buffer->data);
}
