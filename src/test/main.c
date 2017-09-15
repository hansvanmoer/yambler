#include "yambler_type.h"
#include "yambler_parser.h"

#include <stdio.h>

yambler_status read(yambler_char *buffer, size_t buffer_size, size_t *read_count){
	return YAMBLER_OK;
}

int main(int arg_count, const char **args){
	yambler_input_buffer_p buffer;
	yambler_status status = YAMBLER_OK;
	if((status = yambler_input_buffer_create(&buffer, 128, &read))){
		printf("unable to create buffer: %d", status);
		return status;
	}
	yambler_input_buffer_destroy(&buffer);
	return 0;
}
