#ifndef YAMBLER_COMMON_OUTPUT_H
#define YAMBLER_COMMON_OUTPUT_H

#include "common.h"

status open_output(const char *folder, const char *name);

status output_status();

void increase_header_indent();

status decrease_header_indent();

status write_header_line(const char *text);

status write_header_text(const char *text);

status write_header_string(struct string str);

void increase_source_indent();

status decrease_source_indent();

status write_source_line(const char *text);

status write_source_text(const char *text);

status write_header_string(struct string str);

void close_output();

#endif
