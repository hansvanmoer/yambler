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

struct yambler_parser_stack{
  yambler_parser_handle handle;
  struct yambler_parser_stack *next;
};

struct yambler_parser{
	yambler_input_buffer_p input;
	int opened;
	
	struct yambler_parser_event *event;
	int event_ready;

	struct yambler_parser_error error;

	int match;

	struct{
		yambler_char *begin;
		yambler_char *end;
		yambler_char *current;
	} capture;
	
	struct yambler_parser_stack *stack;
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

static yambler_status push_handle(yambler_parser_p parser, yambler_parser_handle handle);

static yambler_status push_handle_pair(yambler_parser_p parser, yambler_parser_handle head, yambler_parser_handle tail);

static yambler_parser_handle pop_handle(yambler_parser_p parser);

static void clear_handle_stack(yambler_parser_p parser);

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

	parser->stack = NULL;
	yambler_status status = push_handle_pair(parser, &parse_begin, &parse_end);
	if(status){
	  return status;
	}
	
	parser->event_ready = 0;
	parser->opened = 1;
	
	parser->error.line = 0;
	parser->error.column = 0;
	parser->error.message = "";

	parser->capture.current = parser->capture.begin;
	
	return yambler_input_buffer_open(input);
}

	
yambler_status yambler_parser_parse(yambler_parser_p parser, struct yambler_parser_event *event){
  assert(parser != NULL);
  assert(event != NULL);
  if(parser->stack){
    parser->event_ready = 0;
    parser->event = event;
    while(!parser->event_ready){
      yambler_parser_handle handle = pop_handle(parser);
      if(handle){
	yambler_status status = (*handle)(parser);
	if(status){
	  return status;
	}
      }else{
	break;
      }
    }
    event = parser->event;
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
	  clear_handle_stack(parser);
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

static yambler_status push_handle(yambler_parser_p parser, yambler_parser_handle handle){
  assert(parser != NULL);
  assert(handle != NULL);

  struct yambler_parser_stack *new_head = malloc(sizeof(struct yambler_parser_stack));
  if(new_head == NULL){
    return YAMBLER_ALLOC_ERROR;
  }
  new_head->next = parser->stack;
  new_head->handle = handle;
  parser->stack = new_head;
  return YAMBLER_OK;
}

static yambler_status push_handle_pair(yambler_parser_p parser, yambler_parser_handle head, yambler_parser_handle tail){
  assert(parser != NULL);
  assert(head != NULL);
  assert(tail != NULL);
  struct yambler_parser_stack *new_tail = malloc(sizeof(struct yambler_parser_stack));
  if(new_tail == NULL){
    return YAMBLER_ALLOC_ERROR;
  }
  new_tail->next = parser->stack;
  new_tail->handle = tail;
  struct yambler_parser_stack *new_head = malloc(sizeof(struct yambler_parser_stack));
  if(new_head == NULL){
    free(new_tail);
    return YAMBLER_ALLOC_ERROR;
  }
  new_head->next = new_tail;
  new_head->handle = head;
  parser->stack = new_head;
  return YAMBLER_OK;
}

static yambler_parser_handle pop_handle(yambler_parser_p parser){
  struct yambler_parser_stack *old_head = parser->stack;
  yambler_parser_handle handle = old_head->handle;
  parser->stack = old_head->next;
  free(old_head);
  return handle;
}			    

static void clear_handle_stack(yambler_parser_p parser){
  struct yambler_parser_stack *head = parser->stack;
  while(head){
    struct yambler_parser_stack *old_head = head;
    head = head->next;
    free(old_head);
  }
}

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
  parser->event->type = YAMBLER_PE_DOCUMENT_BEGIN;
  parser->event_ready = 1;
  push_handle(parser, &parse);
  return YAMBLER_OK;
}

static yambler_status parse_comment(yambler_parser_p parser){
  yambler_status status = capture_until_pred(parser, &match_newline);
  switch(status){
  case YAMBLER_OK:
    pop_char(parser, LINE_FEED_CHAR);
  case YAMBLER_EMPTY:
    parser->event->type = YAMBLER_PE_COMMENT;
    deliver_capture(parser);
    parser->event_ready = 1;
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
  if(status){
    return status;
  }
  switch(c){
  case COMMENT_CHAR:
    status = push_handle_pair(parser, &parse_comment, &parse);
    if(status){
      return status;
    }
    pop_char(parser, c);
    return YAMBLER_OK;
  default:
    parser->error.message = "unexpected character";
    return YAMBLER_SYNTAX_ERROR;
  }
};

static yambler_status parse_end(yambler_parser_p parser){
  parser->event->type = YAMBLER_PE_DOCUMENT_END;
  parser->event_ready = 1;
  return YAMBLER_OK;
}
