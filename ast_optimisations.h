#ifndef AST_OPTIMISATIONS_H
#define AST_OPTIMISATIONS_H

#include "ast_visitor.h"
#include "ast_executor.h"
#include "stdbool.h"

void optimise_ast(expression* ast);

#endif