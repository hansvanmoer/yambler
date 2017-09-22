#ifndef YAMBLER_IO_H
#define YAMBLER_IO_H

#include "yambler_type.h"
#include "yambler_decoder.h"
#include "yambler_encoder.h"

#include <stddef.h>

yambler_status open_binary_file_for_read(yambler_decoder_state *state);

yambler_status open_binary_file_for_write(yambler_encoder_state *state);

void close_binary_file(yambler_decoder_state *state);

yambler_status binary_read(yambler_decoder_state state, yambler_byte *buffer, size_t buffer_size, size_t *read_count);

#endif
