#include "common.h"

#include <assert.h>
#include <stdlib.h>

const char *status_message(status status_){
  switch(status_){
  case STATUS_OK:
    return "ok";
  case STATUS_ALLOC_ERROR:
    return "memory allocation failed";
  case STATUS_IO_ERROR:
    return "I/O error";
  case STATUS_EMPTY:
    return "no more tokens";
  case STATUS_SYNTAX_ERROR:
    return "syntax error";
  case STATUS_PATH_TOO_LONG:
    return "path too long";
  case STATUS_MISMATCHED_INDENTATION:
    return "mismatched indentation";
  case STATUS_SUBSTITUTION_FAILED:
    return "substitution failed";
  case STATUS_ERROR:
  default:
    return "unspecified error";    
  }
}

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

void string_print(FILE *file, struct string str){
  fwrite(str.data, sizeof(char), str.length, file);
}

void string_print_quoted(FILE *file, struct string str){
  fputc('\'', file);
  string_print(file, str);
  fputc('\'', file);
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
