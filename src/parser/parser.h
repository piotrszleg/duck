#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../utility.h"
#include "../containers/vector.h"
#include "../error/error.h"
#include "ast/ast.h"

Expression* parse_string(const char* s);
Expression* parse_file(const char* file);
char* get_source_line(const char* file, int line);

#endif