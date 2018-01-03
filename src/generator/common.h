#ifndef YAMBLER_GENERATOR_COMMON_H
#define YAMBLER_GENERATOR_COMMON_H

#include <stddef.h>
#include <stdio.h>

enum status_code{
  STATUS_OK = 0,
  STATUS_ERROR,
  STATUS_ALLOC_ERROR,
  STATUS_IO_ERROR,
  STATUS_EMPTY,
  STATUS_SYNTAX_ERROR,
  STATUS_PATH_TOO_LONG,
  STATUS_MISMATCHED_INDENTATION,
  STATUS_SUBSTITUTION_FAILED
};

typedef enum status_code status;

const char *status_message(status status_);

struct string{
  char *data;
  size_t length;
};

status string_create(struct string *str, size_t length);

void string_print(FILE *file, struct string str);

void string_print_quoted(FILE *file, struct string str);

void string_destroy(struct string *str);

struct buffer{
  char *data;
  size_t length;
  size_t size;
};

status buffer_create(struct buffer *buffer, size_t size);

void buffer_destroy(struct buffer *buffer);

#endif
