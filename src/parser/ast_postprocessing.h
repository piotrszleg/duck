#ifndef AST_POSTPROCESSING_H
#define AST_POSTPROCESSING_H

#include "ast_visitor.h"
#include "../containers/map.h"
#include "../containers/vector.h"

void postprocess_ast(Expression** ast);

#endif