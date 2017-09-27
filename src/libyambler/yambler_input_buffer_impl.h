#ifndef YAMBLER_INPUT_BUFFER_IMPL_H
#define YAMBLER_INPUT_BUFFER_IMPL_H

#include "yambler_input_buffer.h"

yambler_status yambler_input_buffer_open(yambler_input_buffer_p buffer);

yambler_status yambler_input_buffer_peek(yambler_input_buffer_p buffer, yambler_char *dest);

void yambler_input_buffer_pop(yambler_input_buffer_p buffer);

yambler_status yambler_input_buffer_get(yambler_input_buffer_p buffer, yambler_char *dest);

void yambler_input_buffer_close(yambler_input_buffer_p buffer);

#endif
