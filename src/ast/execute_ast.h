#ifndef AST_EXECUTOR_H
#define AST_EXECUTOR_H

#include <stdbool.h>
#include "../utility.h"
#include "../parser/ast.h"
#include "../object_system/object.h"
#include "../transformers/bytecode_to_myjit.h"

typedef struct {
    ManagedPointer mp;
    Expression* body;
} ASTSourcePointer;

typedef struct ASTExecutionState ASTExecutionState;
struct ASTExecutionState {
    bool returning;
    vector used_objects;
};

void ast_execution_state_init(ASTExecutionState* state);
void ast_execution_state_deinit(ASTExecutionState* state);
Object execute_ast(Executor* E, Expression* expression, bool keep_scope);

#include "../error/error.h"
#include "../error/execution_state.h"
#include "../runtime/builtins.h"

#endif