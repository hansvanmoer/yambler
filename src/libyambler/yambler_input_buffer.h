#ifndef YAMBLER_INPUT_BUFFER_H
#define YAMBLER_INPUT_BUFFER_H

#include "yambler_type.h"
#include "yambler_decoder.h"

#include <stddef.h>

typedef void * yambler_input_buffer_state;

typedef yambler_status (*yambler_input_buffer_open_callback)(yambler_input_buffer_state *);

typedef yambler_status (*yambler_input_buffer_read_callback)(yambler_input_buffer_state , yambler_char *, size_t, size_t *);

typedef void (*yambler_input_buffer_close_callback)(yambler_input_buffer_state *);

struct yambler_input_buffer;

typedef struct yambler_input_buffer * yambler_input_buffer_p;

yambler_status yambler_input_buffer_create(yambler_input_buffer_p *dest, size_t initial_size, yambler_input_buffer_state state, yambler_input_buffer_read_callback read, yambler_input_buffer_open_callback open, yambler_input_buffer_close_callback close);

yambler_status yambler_input_buffer_create_with_decoder(yambler_input_buffer_p *dest, size_t initial_size, yambler_decoder_p decoder);

void yambler_input_buffer_destroy(yambler_input_buffer_p *src);

void yambler_input_buffer_destroy_all(yambler_input_buffer_p *buffer_src, yambler_decoder_p *decoder_src);

#endif
