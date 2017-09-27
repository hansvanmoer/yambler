#include "yambler_input_buffer.h"
#include "yambler_input_buffer_impl.h"
#include "yambler_parser.h"

#include <assert.h>
#include <stdlib.h>

#define COMMENT_CHAR 0x23
#define SPACE_CHAR 0x20
#define TAB_CHAR 0x09
#define LINE_FEED_CHAR 0x0A
#define CARRIAGE_RETURN_CHAR 0x0D

#define CAPTURE_INITIAL_SIZE 128
#define CAPTURE_SIZE_INCREMENT 1024

/*
 * definition of the parser struct and function types
 */

typedef yambler_status (*yambler_parser_handle)(yambler_parser_p);

typedef int (*yambler_predicate)(yambler_char);

struct yambler_parser{
	yambler_input_buffer_p input;
	int opened;
	
	struct yambler_parser_event *event;
	int done;

	struct yambler_parser_error error;

	int match;

	struct{
		yambler_char *begin;
		yambler_char *end;
		yambler_char *current;
	} capture;
	
	yambler_parser_handle next;
};

/*
 * forward declarations of utility functions
 */

static yambler_status get_char(yambler_parser_p parser, yambler_char *dest);

static yambler_status peek_char(yambler_parser_p parser, yambler_char *dest);

static void pop_char(yambler_parser_p parser, yambler_char peeked);

static void reset_capture(yambler_parser_p parser);

static yambler_status capture(yambler_parser_p parser, yambler_char c);

static void deliver_capture(yambler_parser_p parser);

static yambler_status skip_none_or_more_pred(yambler_parser_p parser, yambler_predicate pred);

static yambler_status capture_until_pred(yambler_parser_p parser, yambler_predicate pred);

/*
 * forward declarations of matchers
 */

static int match_non_breaking_whitespace(yambler_char c);

static int match_newline(yambler_char c);

/*
 * forward declarations of parser functions
 */

static yambler_status parse_begin(yambler_parser_p parser);

static yambler_status parse_comment(yambler_parser_p parser);

static yambler_status parse(yambler_parser_p parser);

static yambler_status parse_end(yambler_parser_p parser);


/*
 * implementation of parser lifecycle functions
 */

yambler_status yambler_parser_create(yambler_parser_p *dest){
	assert(dest != NULL);
	
	yambler_parser_p parser = malloc(sizeof(struct yambler_parser));
	if(parser == NULL){
		return YAMBLER_ALLOC_ERROR;
	}

	parser->capture.begin = malloc(sizeof(yambler_char) * CAPTURE_INITIAL_SIZE);
	if(parser->capture.begin == NULL){
		free(parser);
		return YAMBLER_ALLOC_ERROR;
	}
	parser->capture.end = parser->capture.begin + CAPTURE_INITIAL_SIZE;
	
	parser->opened = 0;
    
	*dest = parser;
	
	return YAMBLER_OK;
}

yambler_status yambler_parser_open(yambler_parser_p parser, yambler_input_buffer_p input){
	assert(parser != NULL);
	
	if(parser->opened){
	  yambler_parser_close(parser);
	}
	
	parser->input = input;
	parser->next = &parse_begin;
	parser->done = 0;
	parser->opened = 1;
	
	parser->error.line = 0;
	parser->error.column = 0;
	parser->error.message = "";

	parser->capture.current = parser->capture.begin;
	
	return yambler_input_buffer_open(input);
}

	
yambler_status yambler_parser_parse(yambler_parser_p parser, struct yambler_parser_event *event){
	assert(parser != NULL);
	if(parser->next){
		parser->done = 0;
		parser->event = event;
		while(!parser->done){
			yambler_status status = (*parser->next)(parser);
			if(status){
				return status;
			}
		}
		return YAMBLER_OK;
	}
	return YAMBLER_EMPTY;
}

int yambler_parser_get_error(yambler_parser_p parser, struct yambler_parser_error *error){
	if(parser->error.message != '\0'){
		*error = parser->error;
		return 1;
	}else{
		return 0;
	}
}

void yambler_parser_close(yambler_parser_p parser){
	assert(parser != NULL);

	if(parser->opened){
		yambler_input_buffer_close(parser->input);
		parser->opened = 0;
	}
}

void yambler_parser_destroy(yambler_parser_p *src){
	assert(src != NULL);

	yambler_parser_p parser = *src;

	assert(parser);
  
	if(parser->opened){
		yambler_parser_close(*src);
	}

	free(parser->capture.begin);
  
	free(parser);
}

void yambler_parser_destroy_all(yambler_parser_p *parser_src, yambler_input_buffer_p *buffer_src, yambler_decoder_p *decoder_src){
	if(parser_src && *parser_src){
		yambler_parser_destroy(parser_src);
	}
	if(buffer_src && *buffer_src){
		yambler_input_buffer_destroy(buffer_src);
	}
	if(decoder_src && *decoder_src){
		yambler_decoder_destroy(decoder_src);
	}
}

/*
 * implementations of utility functions
 */

static yambler_status get_char(yambler_parser_p parser, yambler_char *dest){
	yambler_char c;
	yambler_status status = yambler_input_buffer_get(parser->input, &c);
	if(status){
		return status;
	}
	if(match_newline(c)){
		parser->error.column = 0;
		++parser->error.line;
	}else{
		++parser->error.column;
	}
}

static yambler_status peek_char(yambler_parser_p parser, yambler_char *dest){
	return yambler_input_buffer_peek(parser->input, dest);
}

static void pop_char(yambler_parser_p parser, yambler_char peeked){
	yambler_input_buffer_pop(parser->input);
	if(match_newline(peeked)){
		parser->error.column = 0;
		++parser->error.line;
	}else{
		++parser->error.column;
	}
}

static void reset_capture(yambler_parser_p parser){
	parser->capture.current = parser->capture.begin;
}

static void deliver_capture(yambler_parser_p parser){
	parser->event->value.begin = parser->capture.begin;
	parser->event->value.length = parser->capture.current - parser->capture.begin;
}

static yambler_status capture(yambler_parser_p parser, yambler_char c){
	*parser->capture.current = c;
	++parser->capture.current;
	if(parser->capture.current == parser->capture.end){
		size_t size = (parser->capture.end - parser->capture.begin);
		size_t new_size = size + CAPTURE_SIZE_INCREMENT;
		yambler_char *new_begin = realloc(parser->capture.begin, new_size * sizeof(yambler_char));
		if(new_begin == NULL){
			return YAMBLER_ALLOC_ERROR;
		}
		parser->capture.begin = new_begin; 
		parser->capture.current = new_begin + size;
		parser->capture.end = new_begin + new_size;
	}
	return YAMBLER_OK;
}


static yambler_status skip_none_or_more_pred(yambler_parser_p parser, yambler_predicate pred){
	yambler_char c;
	do{
		yambler_status status = peek_char(parser, &c);
		if(status){
			return status;
		}
		if(!((*pred)(c))){
			return YAMBLER_OK;
		}
		pop_char(parser, c);
	}while(1);
}

static yambler_status capture_until_pred(yambler_parser_p parser, yambler_predicate pred){
	reset_capture(parser);
	yambler_char c;
	do{
		yambler_status status = peek_char(parser, &c);
		if(status){
			return status;
		}
		if((*pred)(c)){
			return YAMBLER_OK;
		}
		status = capture(parser, c);
		if(status){
			return status;
		}
		pop_char(parser, c);
	}while(1);
}

/*
 * implementation of matchers
 */

static int match_non_breaking_whitespace(yambler_char c){
	return c == TAB_CHAR || c == SPACE_CHAR;
}

static int match_newline(yambler_char c){
	return c == LINE_FEED_CHAR || c == CARRIAGE_RETURN_CHAR;
}

/*
 * implementation of parser functions
 */

static yambler_status parse_begin(yambler_parser_p parser){
	parser->next = &parse;
	parser->done = 1;
	parser->event->type = YAMBLER_PE_DOCUMENT_BEGIN;
	return YAMBLER_OK;
}

static yambler_status parse_comment(yambler_parser_p parser){
	yambler_status status = capture_until_pred(parser, &match_newline);
	switch(status){
	case YAMBLER_OK:
		get_char(parser, NULL);
	case YAMBLER_EMPTY:
		parser->event->type = YAMBLER_PE_COMMENT;
		deliver_capture(parser);
		parser->done = 1;
		parser->next = &parse;
	}
	return status;
}

static yambler_status parse(yambler_parser_p parser){
	yambler_status status = skip_none_or_more_pred(parser, &match_non_breaking_whitespace);
	if(status){
		return status;
	}
	yambler_char c;
	status = peek_char(parser, &c);
	switch(status){
	case YAMBLER_EMPTY:
		parser->next = &parse_end;
		return YAMBLER_OK;
	case YAMBLER_OK:
		break;
	default:
		return status;
	}
	switch(c){
	case COMMENT_CHAR:
		parser->next = &parse_comment;
		pop_char(parser, c);
		return YAMBLER_OK;
	default:
		parser->error.message = "unexpected character";
		return YAMBLER_SYNTAX_ERROR;
	}
};

static yambler_status parse_end(yambler_parser_p parser){
	parser->next = NULL;
	parser->done = 1;
	parser->event->type = YAMBLER_PE_DOCUMENT_END;
	return YAMBLER_OK;
}
