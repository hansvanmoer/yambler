#include "yambler_type.h"
#include "yambler_encoder.h"
#include "yambler_decoder.h"

#include <stdlib.h>
#include <stdio.h>

#include <getopt.h>
#include <linux/limits.h>

static char input_path[PATH_MAX];

yambler_status open_binary_input(yambler_decoder_state *state){
	FILE *file;
	if(input_path[0] == '\0'){
		file = stdin;
	}else{
		file = fopen(input_path, "rb");
	}
	if(file == NULL){
		return YAMBLER_ERROR;
	}else{
		*state = (yambler_decoder_state)file;
		return YAMBLER_OK;
	}
}

void close_binary_input(yambler_decoder_state *state){
	FILE *file = (FILE *)*state;
	if(file != stdin){
		fclose(file);
	}
	*state = NULL;
};

yambler_status read(yambler_decoder_state state, yambler_byte *buffer, size_t buffer_size, size_t *read_count){
	FILE *file = (FILE *)state;
	*read_count = fread(buffer, sizeof(yambler_byte), buffer_size, file);
	return YAMBLER_OK;
}

int main(int arg_count, const char **args){
	
	

	return 0;
}
