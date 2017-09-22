#include "yambler_type.h"
#include "yambler_encoder.h"
#include "yambler_decoder.h"

#include "options.h"

#include <stdlib.h>
#include <stdio.h>

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
	printf("read count: %d\n",(int)*read_count); 
	return YAMBLER_OK;
}

yambler_status decode(){
	yambler_decoder_p decoder;
	yambler_status status = yambler_decoder_create(&decoder, buffer_size * 4, input_encoding, &binary_read, NULL, &open_binary_file_for_read, &close_binary_file);
	if(status){
		printf("unable to create decoder\n");
		return status;
	}
	
	yambler_char *buffer = malloc(sizeof(yambler_char) * buffer_size);
	if(buffer == NULL){
		yambler_decoder_destroy(&decoder);
		return YAMBLER_ALLOC_ERROR;
	}
	FILE *file = fopen(output_path, "wb");
	if(file == NULL){
		printf("unable to open output file '%s'\n", output_path);
		yambler_decoder_destroy(&decoder);
		return YAMBLER_ERROR;
	}

	status = yambler_decoder_open(decoder);
	if(status){
		printf("unable to open file '%s'\n", input_path);
		return status;
	}

	size_t count;
	while((status = yambler_decoder_decode(decoder,  buffer, buffer_size, &count)) == YAMBLER_OK){
		printf("output count: %d\n", (int)count);
		if(count == 0){
			break;
		}
		for(size_t i = 0; i < count; ++i){
			printf("%d\n", (int)buffer[i]);
		}
		fwrite(buffer, sizeof(yambler_char), buffer_size, file);
	}

	yambler_decoder_close(decoder);
	yambler_decoder_destroy(&decoder);
	fclose(file);
	return status;
}

yambler_status encode(){
	return YAMBLER_OK;
}

yambler_status execute_action(){
	switch(action){
	case ACTION_DECODE:
		return decode();
	case ACTION_ENCODE:
		return encode();
	default:
		return YAMBLER_ERROR;
	}
}

int main(int arg_count, char * const args[]){
	yambler_status status = parse_options(arg_count, args);
	if(status){
		return status;
	}
	if(mode == MODE_INTERACTIVE){
		parse_interactive();
	}
	if(verbosity == VERBOSITY_VERBOSE){
		print_options();
	}
	if(action == ACTION_NONE){
		return 0;
	}else{
		status = execute_action();
		if(status){
			printf("error: %s\n", yambler_status_message(status));
		}
		return (int)status;
	}
}
