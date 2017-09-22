#include "yambler_type.h"
#include "yambler_encoder.h"
#include "yambler_decoder.h"

#include "options.h"
#include "io.h"

#include <stdlib.h>
#include <stdio.h>

yambler_status decode(){
	yambler_decoder_p decoder;
	yambler_status status = yambler_decoder_create(&decoder, buffer_size * 4, input_encoding, &binary_read, NULL, &open_binary_file_for_read, &close_binary_file);
	if(status){
		fprintf(stderr, "unable to create decoder\n");
		return status;
	}
	
	yambler_char *buffer = malloc(sizeof(yambler_char) * buffer_size);
	if(buffer == NULL){
		fprintf(stderr, "unable to allocate buffer");
		yambler_decoder_destroy(&decoder);
		return YAMBLER_ALLOC_ERROR;
	}
	FILE *file = fopen(output_path, "wb");
	if(file == NULL){
		fprintf(stderr,"unable to open output file '%s'\n", output_path);
		free(buffer);
		yambler_decoder_destroy(&decoder);
		return YAMBLER_ERROR;
	}

	status = yambler_decoder_open(decoder);
	if(status){
		fprintf(stderr,"unable to open file '%s'\n", input_path);
		return status;
	}

	size_t count;
	while((status = yambler_decoder_decode(decoder,  buffer, buffer_size, &count)) == YAMBLER_OK){
		if(count == 0){
			break;
		}
		fwrite(buffer, sizeof(yambler_char), count, file);
	}

	yambler_decoder_close(decoder);
	yambler_decoder_destroy(&decoder);
	fclose(file);
	return status;
}

yambler_status encode(){
	yambler_encoder_p encoder;

	yambler_status status = yambler_encoder_create(&encoder, buffer_size * 4, output_encoding, encoder_flags, &binary_write, NULL, &open_binary_file_for_write , &close_binary_file);
	if(status){
		fprintf(stderr,"unable to create encoder\n");
		return status;
	}

	yambler_char *buffer = malloc(sizeof(yambler_byte) * buffer_size * 4);
	if(buffer == NULL){
		fprintf(stderr, "unable to allocate buffer\n");
		yambler_encoder_destroy(&encoder);
		return YAMBLER_ALLOC_ERROR;
	}

	FILE *file = fopen(input_path, "rb");
	if(file == NULL){
		fprintf(stderr,"unable to open input file '%s'\n", input_path);
		free(buffer);
		yambler_encoder_destroy(&encoder);
	}

	status = yambler_encoder_open(encoder);
	if(status){
		fprintf(stderr, "unable to open encoder\n");
		return status;
	}
	
	while(1){
		size_t count = fread(buffer, sizeof(yambler_char) ,buffer_size * 4, file);
		if(count == 0){
			break;
		}
		size_t write_count;
		status = yambler_encoder_encode(encoder, buffer, count, &write_count);
		if(status || write_count == 0){
			break;
		}
	}
	yambler_encoder_close(encoder);
	fclose(file);
	free(buffer);
	yambler_encoder_destroy(&encoder);
	return status;
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
