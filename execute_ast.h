#ifndef AST_EXECUTOR_H
#define AST_EXECUTOR_H

#include "parser/ast.h"
#include "object_system/object.h"
#include "error/error.h"
#include "runtime/builtins.h"
#include "stdbool.h"
#include "error/execution_state.h"
#include "utility.h"

typedef struct {
    ManagedPointer mp;
    expression* body;
} ASTSourcePointer;

void ast_source_pointer_destructor(Executor* E, ASTSourcePointer* sp);

Object execute_ast(Executor* E, expression* exp, Object scope, int keep_scope);

#endif