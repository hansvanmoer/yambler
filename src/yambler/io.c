#include "io.h"

#include "options.h"

#include <stdio.h>
#include <stdlib.h>

yambler_status open_binary_file_for_read(yambler_decoder_state *state){
	FILE *file = fopen(input_path, "rb");
	if(file == NULL){
		fprintf(stderr, "unable to open file for read '%s'\n", input_path);
		return YAMBLER_ERROR;
	}else{
		*state = (yambler_decoder_state)file;
		return YAMBLER_OK;
	}
}

yambler_status open_binary_file_for_write(yambler_encoder_state *state){
	FILE *file = fopen(output_path, "wb");
	if(file == NULL){
		fprintf(stderr, "unable to open file for write '%s'\n", output_path);
		return YAMBLER_ERROR;
	}else{
		*state = (yambler_encoder_state)file;
		return YAMBLER_OK;
	}
}

void close_binary_file(yambler_decoder_state *state){
	FILE *file = (FILE *)*state;
	if(file != stdin){
		fclose(file);
	}
	*state = NULL;
};

yambler_status binary_read(yambler_decoder_state state, yambler_byte *buffer, size_t buffer_size, size_t *read_count){
	FILE *file = (FILE *)state;
	*read_count = fread(buffer, sizeof(yambler_byte), buffer_size, file);
	return YAMBLER_OK;
}
