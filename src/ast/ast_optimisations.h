#ifndef AST_OPTIMISATIONS_H
#define AST_OPTIMISATIONS_H

#include "../parser/ast_visitor.h"
#include "../transformers/expression_object.h"
#include "execute_ast.h"
#include "../execution/options.h"
#include "stdbool.h"

void optimise_ast(Executor* E, Expression** ast);

#endif