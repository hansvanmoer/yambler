#include "common.h"

#include <assert.h>
#include <stdlib.h>

status string_create(struct string *str, size_t length){
  assert(str != NULL);
  char *buffer = malloc(length + 1);
  if(buffer == NULL){
    return STATUS_ALLOC_ERROR;
  }
  buffer[length] = '\0';
  str->data = buffer;
  str->length = length;
  return STATUS_OK;
}

void string_destroy(struct string *str){
  assert(str != NULL);
  free(str->data);
}


status buffer_create(struct buffer *buffer, size_t size){
  assert(buffer != NULL);
  char *nbuffer = malloc(size);
  if(nbuffer == NULL){
    return STATUS_ALLOC_ERROR;
  }
  buffer->data = nbuffer;
  buffer->size = size;
  buffer->length = 0;
  return STATUS_OK;
}

void buffer_destroy(struct buffer *buffer){
  assert(buffer != NULL);
  free(buffer->data);
}
