#ifndef AST_EXECUTOR_H
#define AST_EXECUTOR_H

#include "parser/ast.h"
#include "object_system/object.h"

typedef struct {
    ManagedPointer mp;
    expression* body;
} ASTSourcePointer;

typedef struct ASTExecutionState ASTExecutionState;
struct ASTExecutionState {
    bool returning;
    vector used_objects;
};

void ast_execution_state_init(ASTExecutionState* state);
void ast_execution_state_deinit(ASTExecutionState* state);
Object execute_ast(Executor* E, expression* exp, Object scope, int keep_scope);

#include "error/error.h"
#include "runtime/builtins.h"
#include "stdbool.h"
#include "error/execution_state.h"
#include "utility.h"

#endif