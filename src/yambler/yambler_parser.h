#ifndef YAMBLER_PARSER_H
#define YAMBLER_PARSER_H

#include "yambler_type.h"

#include <stddef.h>

struct yambler_input_buffer;

typedef struct yambler_input_buffer * yambler_input_buffer_p;

typedef yambler_status (*yambler_read)(yambler_char *, size_t, size_t *);

typedef yambler_status (*yambler_read_binary)(yambler_byte *, size_t, size_t *);

yambler_status yambler_input_buffer_create(yambler_input_buffer_p *buffer_p, size_t initial_size, yambler_read read_callback);

yambler_status yambler_input_buffer_create_binary(yambler_input_buffer_p *buffer_p, size_t initial_size, yambler_read_binary read_callback, const char *encoding_name);

yambler_status yambler_input_buffer_open(yambler_input_buffer_p buffer);

yambler_status yambler_input_buffer_reserve(yambler_input_buffer_p buffer, size_t length);

yambler_status yambler_input_buffer_get(yambler_input_buffer_p buffer, yambler_char *dest);

void yambler_input_buffer_destroy(yambler_input_buffer_p *buffer);

struct yambler_parser;

typedef struct yambler_parser * yambler_parser_p;

#endif
