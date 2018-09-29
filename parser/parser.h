#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

typedef void (*parsing_handler_pointer)(expression*); 

void parse_file(char* file_name, parsing_handler_pointer callback);

#endif