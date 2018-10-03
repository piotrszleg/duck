#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

expression* parsing_result;

void parse_string(char* s);
void parse_file(char* file_name);

#endif