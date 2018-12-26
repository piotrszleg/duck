#ifndef AST_TO_BYTECODE_H
#define AST_TO_BYTECODE_H

#include "parser/ast.h"
#include "bytecode.h"
#include "error/error.h"
#include "datatypes/stream.h"
#include "datatypes/stack.h"
#include <stdbool.h>

bytecode_program ast_to_bytecode(expression* exp, int keep_scope);
void optimise_bytecode(bytecode_program* prog);

#endif