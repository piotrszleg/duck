#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../datatypes/vector.h"
#include "../error/error.h"
#include "ast.h"

expression* parsing_result;

void parse_string(const char* s);
void parse_file(const char* file_name);

#endif