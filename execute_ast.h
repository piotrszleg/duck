#ifndef AST_EXECUTOR_H
#define AST_EXECUTOR_H

#include "parser/ast.h"
#include "object_system/object.h"
#include "error/error.h"
#include "builtins.h"
#include "stdbool.h"
#include "error/execution_state.h"

typedef struct {
    int line_number;
    int column_number;
    bool returning;
} ast_executor_state;

object execute_ast(ast_executor_state* state, expression* exp, object scope, int keep_scope);

#endif