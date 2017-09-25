#include "yambler_input_buffer.h"
#include "yambler_input_buffer_impl.h"

#include <assert.h>
#include <stdlib.h>

struct yambler_input_buffer{
  yambler_char *data;
  size_t size;
  size_t size_increment;
  size_t length;
  size_t index;

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
  buffer->index = 0;

  buffer->read_state = state;
  buffer->open = open;
  buffer->read = read;
  buffer->close = close;

  *dest = buffer;
  
  return YAMBLER_OK;
}

yambler_status yambler_input_buffer_open(yambler_input_buffer_p buffer){
  if(buffer->open){
    return (*buffer->open)(&buffer->read_state);
  }else{
    return YAMBLER_OK;
  }
}

void yambler_input_buffer_close(yambler_input_buffer_p buffer){
  if(buffer->close){
    (*buffer->close)(&buffer->read_state);
  }
}


void yambler_input_buffer_destroy(yambler_input_buffer_p *src){
  assert(src != NULL);
  
  yambler_input_buffer_p buffer = *src;

  assert(buffer != NULL);

  free(buffer->data);
  free(buffer);

  *src = NULL;
}
