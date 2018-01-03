#include "common.h"
#include "output.h"

#include <stdio.h>
#include <string.h>
#include <linux/limits.h>

static status output_status_ = STATUS_OK;

static FILE *header_file = NULL;

static int header_indent;

static FILE *source_file = NULL;

static int source_indent;

static struct string name;

status output_status(){
  return output_status_;
}

status open_output(const char *folder, const char *name_){
  size_t name_length = strlen(name_);
  output_status_ = string_create(&name, name_length);
  if(output_status_){
    fprintf(stderr, "unable to allocate buffer for name\n");
    return output_status_;
  }
  strcpy(name.data, name_);
  
  char path[PATH_MAX + 1];
  strcpy(path, folder);
  size_t folder_size = strlen(folder);
  size_t size = folder_size + 1 + name_length + 2;
  if(size > PATH_MAX){
    string_destroy(&name);
    return output_status_ = STATUS_PATH_TOO_LONG;
  }
  path[folder_size] = '/';
  strcpy(path+folder_size+1, name_);
  path[size - 2] = '.';
  path[size - 1] = 'h';
  path[size] = '\0';
  header_file = fopen(path, "w+");
  if(header_file == NULL){
    fprintf(stderr, "unable to open output file '%s'\n", path);
    string_destroy(&name);
    return output_status_ = STATUS_IO_ERROR;
  }
  path[size - 1] = 'c';
  source_file = fopen(path, "w+");
  if(source_file == NULL){
    fprintf(stderr, "unable to open output file '%s'\n", path);
    string_destroy(&name);
    fclose(header_file);
    return output_status_ = STATUS_IO_ERROR;
  }
  return output_status_;
}

void increase_header_indent(){
  ++header_indent;
}

status decrease_header_indent(){
  if(header_indent == 0){
    fprintf(stderr, "mismatched header indentation: < 0\n");
    return output_status_ = STATUS_MISMATCHED_INDENTATION;
  }
  --header_indent;
  return STATUS_OK;
}

void increase_source_indent(){
  ++source_indent;
}

status decrease_source_indent(){
  if(source_indent == 0){
    fprintf(stderr, "mismatched source indentation: < 0\n");
    return output_status_ = STATUS_MISMATCHED_INDENTATION;
  }
  --source_indent;
  return STATUS_OK;
}


static status write_to_file(FILE *file, const char *data, size_t size){
  const char *end = data + size;
  while(1){
    if(data == end){
      return output_status_ = STATUS_OK;
    }
    const char *found = strchr(data, '$');
    if(found == NULL){
      fwrite(data, sizeof(char), found - data, file);
      data = end;
    }else{
      fwrite(data, sizeof(char), found - data, file);
      ++data;
      size_t name_size = 0;
      const char *name_begin = data;
      while(1){
	if(data == end){
	  fprintf(stderr, "unterminated substitution label\n");
	  return output_status_ = STATUS_SUBSTITUTION_FAILED;
	}else if(*data == '$'){
	  ++data;
	  break;
	}else{
	  ++name_size;
	}
      }
      if(name_size == 4 && memcmp(name_begin, "name", 4)){
	fwrite(name.data, sizeof(char), name.length, file);
      }else{
	fprintf(stderr, "invalid substitution label: '");
	fwrite(name_begin, sizeof(char), name_size, stderr);
	fprintf(stderr, "'\n\tpossible labels: 'name'\n");
	return output_status_ = STATUS_SUBSTITUTION_FAILED;
      }
    }
  }
  return output_status_;
}

status write_header_text(const char *text){
  if(output_status_){
    return output_status_;
  }else{
    return write_to_file(header_file, text, strlen(text));
  }
}

status write_source_text(const char *text){
  if(output_status_){
    return output_status_;
  }else{
    return write_to_file(source_file, text, strlen(text));
  }
}

status write_header_string(struct string str){
  if(output_status_){
    return output_status_;
  }else{
    return write_to_file(header_file, str.data, str.length);
  }
}

status write_source_string(struct string str){
  if(output_status_){
    return output_status_;
  }else{
    return write_to_file(source_file, str.data, str.length);
  }
}

static status write_line_to_file(FILE *file, int indent, const char *text){
  if(output_status_){
    return output_status_;
  }
  for(int i = 0; i < indent; ++i){
    if(fputc('\t', file) != '\t'){
      fprintf(stderr, "unable to write indentation to file\n");
      return output_status_ = STATUS_IO_ERROR;
    }
  }
  if(!write_to_file(file, text, strlen(text))){
    if(fputc('\n', file) != '\n'){
      fprintf(stderr, "unable to write newline to file\n");
      return output_status_ = STATUS_IO_ERROR;
    }
  }
  return output_status_;
}

status write_header_line(const char *text){
  return write_line_to_file(header_file, header_indent, text);
}

status write_source_line(const char *text){
  return write_line_to_file(source_file, source_indent, text);
}

void close_output(){
  fclose(source_file);
  fclose(header_file);
  string_destroy(&name);
}


