#include "yambler_input_buffer.h"
#include "yambler_input_buffer_impl.h"
#include "yambler_parser.h"
#include "yambler_parser_impl.h"

#include <assert.h>
#include <stdlib.h>

yambler_status yambler_parser_create(yambler_parser_p *dest){
  assert(dest != NULL);

  yambler_parser_p parser = malloc(sizeof(struct yambler_parser));
  if(parser == NULL){
    return YAMBLER_ALLOC_ERROR;
  }

  parser->input = NULL;

  parser->next = NULL;
  parser->done = 0;

  parser->message = "";
  parser->line = 0;
  parser->column = 0;
  
  *dest = parser;

  return YAMBLER_OK;
}

static yambler_status parser_init(yambler_parser_p parser){
  return YAMBLER_OK;
}

yambler_status yambler_parser_open(yambler_parser_p parser, yambler_input_buffer_p input){
  assert(parser != NULL);

  parser->input = input;
  parser->next = &parse_begin;
  parser->done = 0;
  
  parser->message = "";
  parser->line = 0;
  parser->column = 0;
  
  return yambler_input_buffer_open(input);
}

yambler_status parse_begin(yambler_parser_p parser){
  parser->next = &parse_end;
  parser->done = 1;
  parser->event->type = YAMBLER_PE_DOCUMENT_BEGIN;
  return YAMBLER_OK;
}

yambler_status parse_end(yambler_parser_p parser){
  parser->next = NULL;
  parser->done = 1;
  parser->event->type = YAMBLER_PE_DOCUMENT_END;
  return YAMBLER_OK;
}

yambler_status yambler_parser_parse(yambler_parser_p parser, struct yambler_parser_event *event){
  assert(parser != NULL);
  if(parser->next){
    parser->done = 0;
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

void yambler_parser_close(yambler_parser_p parser){
  assert(parser != NULL);
  
  yambler_input_buffer_open(parser->input);
}

void yambler_parser_destroy(yambler_parser_p *src){
  assert(src != NULL);
  
  yambler_parser_p parser = *src;

  free(parser);
}
