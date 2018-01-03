#include "generator.h"
#include "input.h"
#include "output.h"

#include <stdio.h>

int main(int arg_count, const char **args){
  if(arg_count == 4){
    status result = open_input(args[1]);
    if(result){
      fprintf(stderr, "could not open input-file: %s\n", status_message(result));
      return result;
    }
    struct token token;
    while(1){
      result = next_token(&token);
      if(result == STATUS_EMPTY){
	fprintf(stdout, "end of file reached at line %d\n", input_line());
	break;
      }else if(result){
	fprintf(stderr, "error at line %d\n",input_line());
	break;
      }else{
	switch(token.type){
	case TOKEN_NEWLINE:
	  fprintf(stdout, "new line\n");
	  break;
	case TOKEN_COMMENT:
	  fprintf(stdout, "comment: ");
	  string_print_quoted(stdout, token.text);
	  fprintf(stdout, "\n");
	  break;
	case TOKEN_TEXT:
	  fprintf(stdout, "text: ");
	  string_print_quoted(stdout, token.text);
	  fprintf(stdout, "\n");
	  break;
	case TOKEN_CODE:
	  fprintf(stdout, "code:\n");
	  string_print(stdout, token.text);
	  fprintf(stdout, "\n");
	  break;
	}
      }
    }
    close_input();
    return result;
  }else{
    fprintf(stderr, "wrong number of arguments: expected 4 but got %d\nusage: generator input-file output-folder parser-name\n", arg_count);
    return STATUS_ERROR;
  }

  return STATUS_OK;
}
