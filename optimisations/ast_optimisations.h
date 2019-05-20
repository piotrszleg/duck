#ifndef AST_OPTIMISATIONS_H
#define AST_OPTIMISATIONS_H

#include "../parser/ast_visitor.h"
#include "../execute_ast.h"
#include "stdbool.h"

void optimise_ast(expression* ast);

#endif