#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <linux/limits.h>

int action;

int mode;
char input_path[PATH_MAX + 1];
char output_path[PATH_MAX + 1];

int verbosity = VERBOSITY_SILENT;

size_t buffer_size = DEFAULT_BUFFER_SIZE;

enum yambler_encoding input_encoding = YAMBLER_ENCODING_DETECT;

enum yambler_encoding output_encoding = YAMBLER_ENCODING_UTF_8;

#define OPT_STRING "dv"

static struct option options[] = {
	{"decode",0,NULL,ACTION_DECODE},
	{"verbose",0,NULL, VERBOSITY_VERBOSE},
	{NULL, 0, NULL, 0}
};

yambler_status parse_options(int arg_count, char * const args[]){
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
