#ifndef YAMBLER_PARSER_H
#define YAMBLER_PARSER_H

#include "yambler_input_buffer.h"
#include "yambler_type.h"

#include <stddef.h>

struct yambler_parser;

enum yambler_parser_event_type{
  YAMBLER_PE_DOCUMENT_BEGIN,
  YAMBLER_PE_MAP_BEGIN,
  YAMBLER_PE_MAP_END,
  YAMBLER_PE_DOCUMENT_END,
  YAMBLER_PE_SEQUENCE_BEGIN,
  YAMBLER_PE_SEQUENCE_END,
  YAMBLER_PE_SCALAR,
  YAMBLER_PE_ALIAS,
  YAMBLER_PE_COMMENT,
  YAMBLER_PE_DIRECTIVE
};

struct yambler_parser_event{
  enum yambler_parser_event_type type;
  struct yambler_string value;
};

typedef struct yambler_parser * yambler_parser_p;

yambler_status yambler_parser_create(yambler_parser_p *dest);

yambler_status yambler_parser_open(yambler_parser_p parser, yambler_input_buffer_p input_buffer);

yambler_status yambler_parser_parse(yambler_parser_p parser, struct yambler_parser_event *event);

void yambler_parser_destroy(yambler_parser_p *src);

#endif
