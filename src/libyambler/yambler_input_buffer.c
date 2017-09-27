#include "yambler_input_buffer.h"
#include "yambler_input_buffer_impl.h"

#include <assert.h>
#include <stdlib.h>

struct yambler_input_buffer{
	yambler_char *data;
	size_t size;
	size_t size_increment;
	yambler_char *get;
	size_t length;

	int opened;
	yambler_input_buffer_state read_state;
	yambler_input_buffer_open_callback open;
	yambler_input_buffer_read_callback read;
	yambler_input_buffer_close_callback close;
};

#define DEFAULT_SIZE 1024;
#define DEFAULT_SIZE_INCREMENT 1024

yambler_status yambler_input_buffer_create(yambler_input_buffer_p *dest, size_t initial_size, yambler_input_buffer_state state, yambler_input_buffer_read_callback read, yambler_input_buffer_open_callback open, yambler_input_buffer_close_callback close){

  assert(dest != NULL);

  yambler_input_buffer_p buffer = malloc(sizeof(struct yambler_input_buffer));

  if(initial_size == 0){
    initial_size = DEFAULT_SIZE;
  }

  if(buffer == NULL){
    return YAMBLER_ALLOC_ERROR;
  }

  buffer->data = malloc(sizeof(yambler_char) * initial_size);
  if(buffer->data == NULL){
    free(buffer);
    return YAMBLER_ALLOC_ERROR;
  }
  buffer->size = initial_size;
  buffer->size_increment = DEFAULT_SIZE_INCREMENT;
  buffer->length = 0;
  buffer->get = buffer->data;
  
  buffer->opened = 0;
  buffer->read_state = state;
  buffer->open = open;
  buffer->read = read;
  buffer->close = close;

  *dest = buffer;
  
  return YAMBLER_OK;
}

static yambler_status open_decoder(yambler_input_buffer_state *state){
	assert(state != NULL);
	yambler_decoder_p decoder = (yambler_decoder_p)*state;
	assert(decoder != NULL);
	return yambler_decoder_open(decoder);
}

static yambler_status read_decoder(yambler_input_buffer_state state, yambler_char *buffer, size_t buffer_size, size_t *read_count){
	assert(state != NULL);
	yambler_decoder_p decoder = (yambler_decoder_p)state;
	return yambler_decoder_decode(decoder, buffer, buffer_size, read_count);
}

static void close_decoder(yambler_input_buffer_state *state){
	assert(state != NULL);
	yambler_decoder_p decoder = (yambler_decoder_p)*state;
	yambler_decoder_close(decoder);
}

yambler_status yambler_input_buffer_create_with_decoder(yambler_input_buffer_p *dest, size_t initial_size, yambler_decoder_p decoder){
	assert(decoder != NULL);
	return yambler_input_buffer_create(dest, initial_size, (yambler_input_buffer_state)decoder, &read_decoder, &open_decoder, &close_decoder);
}

yambler_status yambler_input_buffer_open(yambler_input_buffer_p buffer){
	if(buffer->opened){
		yambler_input_buffer_close(buffer);
	}
	buffer->opened = 1;
	buffer->get = buffer->data;
	buffer->length = 0;
	if(buffer->open){
		return (*buffer->open)(&buffer->read_state);
	}else{
		return YAMBLER_OK;
	}
}

size_t calculate_next_size(yambler_input_buffer_p buffer, size_t min_length){
	size_t increment = (min_length - buffer->size);
	return increment % buffer->size_increment == 0 ? buffer->size + increment : buffer->size + ((increment / buffer->size_increment) + 1) * buffer->size_increment;
}

yambler_status yambler_input_buffer_fill(yambler_input_buffer_p buffer){
	if(buffer->read){
		yambler_char *put = buffer->get + buffer->length;
		size_t remainder = (buffer->data + buffer->size) - put;
		size_t read_count;
		
		yambler_status status = (*buffer->read)(buffer->read_state, buffer->get+buffer->length, remainder, &read_count);
		if(status){
			return status;
		}
		buffer->length+=read_count;
	}
	return YAMBLER_OK;
}

static yambler_status yambler_input_buffer_grow(yambler_input_buffer_p buffer, size_t min_length){
	size_t min_growth = min_length - buffer->size;
	size_t growth = (min_growth / buffer->size_increment) * buffer->size_increment;
	if((growth % buffer->size_increment) != 0){
		growth+=buffer->size_increment;
	}
	size_t new_size = buffer->size + growth;
	if(new_size > buffer->size){
		yambler_char *new_data = realloc(buffer->data, new_size);
		if(new_data == NULL){
			return YAMBLER_ALLOC_ERROR;
		}
		buffer->get = new_data + (buffer->get - buffer->data);
		buffer->data = new_data;
		buffer->size = new_size;
	}
	return YAMBLER_OK;
}

static yambler_status yambler_input_buffer_ensure(yambler_input_buffer_p buffer, size_t min_length){
	if(min_length < buffer->size){
		yambler_status status = yambler_input_buffer_grow(buffer, min_length);
		if(status){
			return status;
		}
	}
	return yambler_input_buffer_fill(buffer);
}

yambler_status yambler_input_buffer_peek(yambler_input_buffer_p buffer, yambler_char *dest){
	assert(buffer != NULL);
	assert(dest != NULL);
	assert(buffer->opened);
	if(buffer->length == 0){
		yambler_status status = yambler_input_buffer_ensure(buffer, 1);
		if(status){
			return status;
		}
		if(buffer->length == 0){
			return YAMBLER_EMPTY;
		}
	}
	*dest = *buffer->get;
	return YAMBLER_OK;
}

void yambler_input_buffer_pop(yambler_input_buffer_p buffer){
	assert(buffer != NULL);
	assert(buffer->length != 0);
	++buffer->get;
	--buffer->length;
}

yambler_status yambler_input_buffer_get(yambler_input_buffer_p buffer, yambler_char *dest){
	assert(buffer != NULL);

	if(buffer->length == 0){
		yambler_status status = yambler_input_buffer_ensure(buffer, 1);
		if(status){
			return status;
		}
		if(buffer->length == 0){
			if(dest){
				*dest = '\0';
			}
			return YAMBLER_EMPTY;
		}
	}
	if(dest){
		*dest = *buffer->get;
	}
	++buffer->get;
	--buffer->length;
	return YAMBLER_OK;
}

void yambler_input_buffer_close(yambler_input_buffer_p buffer){
	if(buffer->opened){
		if(buffer->close){
			(*buffer->close)(&buffer->read_state);
		}
		buffer->opened = 0;
	}
}


void yambler_input_buffer_destroy(yambler_input_buffer_p *src){
  assert(src != NULL);
  
  yambler_input_buffer_p buffer = *src;

  assert(buffer != NULL);

  yambler_input_buffer_close(buffer);
  
  free(buffer->data);
  free(buffer);

  *src = NULL;
}

void yambler_input_buffer_destroy_all(yambler_input_buffer_p *buffer_src, yambler_decoder_p *decoder_src){
	if(buffer_src){
		yambler_input_buffer_destroy(buffer_src);
	}
	if(decoder_src){
		yambler_decoder_destroy(decoder_src);
	}
}
