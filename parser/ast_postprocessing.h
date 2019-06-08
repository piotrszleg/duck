#ifndef AST_POSTPROCESSING_H
#define AST_POSTPROCESSING_H

#include "ast_visitor.h"
#include "../datatypes/map.h"
#include "../datatypes/stack.h"

void postprocess_ast(expression* ast);

#endif