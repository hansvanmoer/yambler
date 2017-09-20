#ifndef YAMBLER_ENCODER_H
#define YAMBLER_ENCODER_H

#include "yambler_type.h"

#include <stddef.h>

struct yambler_encoder;

typedef struct yambler_encoder * yambler_encoder_p;

typedef void * yambler_encoder_state;

typedef yambler_status (*yambler_encoder_write_callback)(yambler_encoder_state, const yambler_byte *, size_t, size_t *);

typedef yambler_status (*yambler_encoder_open_callback)(yambler_encoder_state *);

typedef void (*yambler_encoder_close_callback)(yambler_encoder_state *);

typedef int yambler_encoder_flag;

#define YAMBLER_ENCODER_INCLUDE_BOM 0x01

yambler_status yambler_encoder_create(yambler_encoder_p *dest, size_t buffer_size, enum yambler_encoding encoding, yambler_encoder_flag options, yambler_encoder_write_callback write, yambler_encoder_state state, yambler_encoder_open_callback open, yambler_encoder_close_callback close);

yambler_status yambler_encoder_open(yambler_encoder_p encoder);

yambler_status yambler_encoder_encode(yambler_encoder_p encoder, const yambler_char *buffer, size_t buffer_size, size_t *write_count);

void yambler_encoder_close(yambler_encoder_p encoder);

void yambler_encoder_destroy(yambler_encoder_p *src);

#endif
