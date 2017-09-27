#ifndef YAMBLER_OPTIONS_H
#define YAMBLER_OPTIONS_H

#include <stddef.h>

#include "yambler_type.h"
#include "yambler_encoder.h"

#define ACTION_NONE '\0'
#define ACTION_DECODE 'd'
#define ACTION_ENCODE 'e'
#define ACTION_PARSE 'p'

extern int action;

#define MODE_COMMAND 0
#define MODE_INTERACTIVE 1

extern int mode;
extern char input_path[];
extern char output_path[];

#define VERBOSITY_VERBOSE 'v'
#define VERBOSITY_SILENT '\0'

extern int verbosity;

#define DEFAULT_BUFFER_SIZE 1024

extern size_t buffer_size;

extern enum yambler_encoding input_encoding;

extern enum yambler_encoding output_encoding;

extern yambler_encoder_flag encoder_flags;

yambler_status parse_options(int arg_count, char * const args[]);

void parse_interactive();

void print_options();

#endif
