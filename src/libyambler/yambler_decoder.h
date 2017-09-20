#ifndef YAMBLER_DECODER_H
#define YAMBLER_DECODER_H

#include "yambler_type.h"

#include <stddef.h>

struct yambler_decoder;

typedef struct yambler_decoder * yambler_decoder_p;

typedef void * yambler_decoder_state;

typedef yambler_status (*yambler_decoder_open_callback)(yambler_decoder_state *);

typedef yambler_status (*yambler_decoder_read_callback)(yambler_decoder_state, yambler_byte *, size_t, size_t *);

typedef void (*yambler_decoder_close_callback)(yambler_decoder_state *);

yambler_status yambler_decoder_create(yambler_decoder_p *result, size_t buffer_size, enum yambler_encoding encoding, yambler_decoder_read_callback read, yambler_decoder_state state, yambler_decoder_open_callback open, yambler_decoder_close_callback close);

yambler_status yambler_decoder_open(yambler_decoder_p decoder);

yambler_status yambler_decoder_decode(yambler_decoder_p decoder, yambler_char *buffer, size_t buffer_size, size_t *count);

void yambler_decoder_close(yambler_decoder_p decoder);

void yambler_decoder_destroy(yambler_decoder_p *src);

#endif
