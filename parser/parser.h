#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

expression* parsing_result;

void parse_string(const char* s);
void parse_file(const char* file_name);

#endif