#include "yambler_type.h"
#include "yambler_encoder.h"
#include "yambler_decoder.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <getopt.h>
#include <linux/limits.h>

#include <iconv.h>

#define ACTION_NONE '\0'
#define ACTION_DECODE 'd'
#define ACTION_ENCODE 'e'

static int action;

#define MODE_COMMAND 0
#define MODE_INTERACTIVE 1

static int mode;
static char input_path[PATH_MAX + 1];
static char output_path[PATH_MAX + 1];

#define VERBOSITY_VERBOSE 'v'
#define VERBOSITY_SILENT '\0'

static int verbosity = VERBOSITY_SILENT;

#define DEFAULT_BUFFER_SIZE 1024

static size_t buffer_size = DEFAULT_BUFFER_SIZE;

enum yambler_encoding input_encoding = YAMBLER_ENCODING_DETECT;

enum yambler_encoding output_encoding = YAMBLER_ENCODING_UTF_8;

#define OPT_STRING "dv"

static struct option options[] = {
	{"decode",0,NULL,ACTION_DECODE},
	{"verbose",0,NULL, VERBOSITY_VERBOSE},
	{NULL, 0, NULL, 0}
};

static yambler_status parse_options(int arg_count, char * const args[]){
	action = ACTION_NONE;

	mode = MODE_COMMAND;
	verbosity = VERBOSITY_SILENT;
	
	int result;
	while((result = getopt_long(arg_count, args, OPT_STRING, options, NULL)) != -1){
		switch(result){
		case ACTION_DECODE:
			action = ACTION_DECODE;
			break;
		case VERBOSITY_VERBOSE:
			verbosity = VERBOSITY_VERBOSE;
			break;
		default:
			return YAMBLER_ERROR;
		}
	}
	if(arg_count == optind){
		input_path[0] = '\0';
		output_path[0] = '\0';
	}else{
		int input_len = strlen(args[optind]);
		if(input_len > PATH_MAX){
			return YAMBLER_BOUNDS_ERROR;
		}
		strcpy(input_path, args[optind]);
		if(arg_count == (optind + 1)){
			output_path[0] = '\0';
		}else{
			int output_len = strlen(args[optind+1]);
			if(output_len > PATH_MAX){
				return YAMBLER_BOUNDS_ERROR;
			}
			strcpy(output_path, args[optind+1]);
		}
	}
	if(input_path == NULL || output_path == NULL || action == ACTION_NONE){
		mode = MODE_INTERACTIVE;
	}else{
		mode = MODE_COMMAND;
	}
	return YAMBLER_OK;
}

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

void print_options(){
	printf("yambler started with the following options:\n");
	if(mode == MODE_COMMAND){
		printf("mode: command\n");
	}else{
		printf("mode: interactive\n");
	}
	switch(action){
	case ACTION_NONE:
		printf("action: none\n");
		break;
	case ACTION_ENCODE:
		printf("action: encode\n");
		break;
	case ACTION_DECODE:
		printf("action: decode\n");
		break;
	}
	if(input_path == NULL){
		printf("input path: <to be supplied by user>\n");
	}else{
		printf("input path: %s\n", input_path);
	}
	if(output_path == NULL){
		printf("output path: <to be supplied by user>\n");
	}else{
		printf("output path: %s\n", output_path);
	}
}

void parse_interactive(){
	int retry;
	if(input_path[0] == '\0'){
		retry = 1;
		while(retry){
			printf("please enter the path to the input file, followed by RETURN:\n");
			char *result = fgets(input_path, PATH_MAX + 1, stdin);
			if(result != NULL && input_path[0] != '\n'){
				input_path[strlen(input_path)-1] = '\0';
				retry = 0;
			}
		}
	}
	if(output_path[0] == '\0'){
		retry = 1;
		while(retry){
			printf("please enter the path to the output file, followed by RETURN:\n");
			char *result = fgets(output_path, PATH_MAX+1, stdin);
			if(result != NULL && output_path[0] != '\n'){
				output_path[strlen(output_path) - 1] = '\0';
				retry = 0;
			}
		}
	}
	if(action == ACTION_NONE){
		retry = 1;
		char buffer[3];
		while(retry){
		printf("please enter the action to be performed by selecting the character, followed by RETURN, from the following options:\n");
		printf("'d' : decode the input file and store the result into the output file\n");
		printf("'e' : encode the input file and store the result into the output file\n");
			char *result = fgets(buffer, 3, stdin);
			if(result != NULL && buffer[1] == '\n'){
				switch(buffer[0]){
				case ACTION_DECODE:
				case ACTION_ENCODE:
					action = buffer[0];
					retry = 0;
					break;
				default:
					printf("unrecognized action: '%c'\n", buffer[0]); 
				}
			}
		}
	}
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
