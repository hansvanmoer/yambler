#ifndef YAMBLER_PARSER_IMPL_H
#define YAMBLER_PARSER_IMPL_H

#include "yambler_parser.h"

typedef yambler_status (*yambler_parser_handle)(yambler_parser_p);

struct yambler_parser{
  yambler_input_buffer_p input;

  struct yambler_parser_event *event;
  int done;
  
  const char *message;
  int line;
  int column;
  
  yambler_parser_handle next;
};

yambler_status parse_begin(yambler_parser_p parser);

yambler_status parse_end(yambler_parser_p parser);

#endif
