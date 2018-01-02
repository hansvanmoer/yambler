#ifndef YAMBLER_GENERATOR_COMMON_H
#define YAMBLER_GENERATOR_COMMON_H

#include <stddef.h>

enum status_code{
  STATUS_OK = 0,
  STATUS_ERROR,
  STATUS_ALLOC_ERROR,
  STATUS_IO_ERROR
};

typedef enum status_code status;

struct string{
  char *data;
  size_t length;
};

status string_create(struct string *str, size_t length);

void string_destroy(struct string *str);

struct buffer{
  char *data;
  size_t length;
  size_t size;
};

status buffer_create(struct buffer *buffer, size_t size);

void buffer_destroy(struct buffer *buffer);

#endif
