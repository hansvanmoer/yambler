#ifndef YAMBLER_GENERATOR_INPUT_H
#define YAMBLER_GENERATOR_INPUT_H

#include "common.h"

status open_input(const char *path);

enum token_type{
  TOKEN_NEWLINE,
  TOKEN_COMMENT,
  TOKEN_TEXT,
  TOKEN_CODE
};

struct token{
  enum token_type type;
  struct string text;
};

status next_token(struct token *token);

int input_line();

int input_column();

void close_input();

#endif
