#include "input.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static struct buffer input_buffer;
static size_t input_pos;
static int line;
static int column;

static size_t capture_pos;

typedef int (*predicate)(char);

static int is_whitespace(char c){
  return c == ' ' || c == '\t';
}

static int is_newline(char c){
  return c == '\n';
}

static int is_comment_delimiter(char c){
  return c == '#';
}

static int is_text(char c){
  return !is_whitespace(c) && !is_newline(c) && !is_comment_delimiter('#');
}

static char peek(){
  return input_buffer.data[input_pos];
}

static void next_pos(){
  if(is_newline(peek())){
    ++line;
    column = 1;
  }
  ++input_pos;
}

static size_t skip_while(predicate pred){
  size_t start_pos = input_pos;
  while(1){
    if(input_pos == input_buffer.length || !(*pred)(input_buffer.data[input_pos])){
      return input_pos - start_pos;
    }
    next_pos();
  }
}

static size_t skip_until(predicate pred){
  size_t start_pos = input_pos;
  while(1){
    if(input_pos == input_buffer.length || (*pred)(input_buffer.data[input_pos])){
      return input_pos - start_pos;
    }
    next_pos();
  }
}

static void start_capture(){
  capture_pos = input_pos;
}

static void end_capture(struct token *token){
  token->text.data = input_buffer.data + capture_pos;
  token->text.length = input_pos - capture_pos;
};

static int eof(){
  return input_pos == input_pos;
}

status open_input(const char *path){
  FILE *file = fopen(path, "r");
  if(file == NULL){
    fprintf(stderr, "unable to open file '%s'\n", path);
    return STATUS_IO_ERROR;
  }
  if(fseek(file, SEEK_END, 0)){
    fclose(file);
    fprintf(stderr, "unable to determine file size '%s'\n", path);
    return STATUS_IO_ERROR;
  }
  long pos = ftell(file);
  if(pos == -1L){
    fclose(file);
    fprintf(stderr, "unable to determine file size '%s'\n", path);
    return STATUS_IO_ERROR;
  }
  if(fseek(file, SEEK_SET, 0)){
   fclose(file);
    fprintf(stderr, "unable to determine file size '%s'\n", path);
    return STATUS_IO_ERROR;
  }
  status status = buffer_create(&input_buffer, (size_t)pos);
  if(status){
    fclose(file);
    return status;
  }
  size_t read_count = fread(input_buffer.data, 1, input_buffer.size, file);
  fclose(file);
  if(read_count != input_buffer.size){
    fprintf(stderr, "wrong amount of characters read: expected %d, got %d\n", (int)input_buffer.size, (int)read_count);
    return STATUS_IO_ERROR;
  }
  input_buffer.length = input_buffer.size;
  line = 1;
  column = 0;
  return STATUS_OK;
}

status next_token(struct token *token){
  skip_while(&is_whitespace);
  if(eof()){
    return STATUS_EMPTY;
  }else{
    char c = peek();
    if(is_comment_delimiter(c)){
      next_pos();
      start_capture();
      skip_until(&is_newline);
      end_capture(token);
      token->type = TOKEN_COMMENT;
    }else if(is_newline(c)){
      next_pos();
      token->type = TOKEN_NEWLINE;
    }else{
      start_capture();
      if(skip_while(&is_text) == 0){
	fprintf(stderr, "unexpected character at line %d, column %d\n", line, column);
	return STATUS_ERROR;
      }
      end_capture(token);
    }
    return STATUS_OK;
  }
}

void close_input(){
  buffer_destroy(&input_buffer);
}

