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

Object execute_ast(Executor* E, Expression* expression, bool keep_scope);

#include "../error/error.h"
#include "../error/execution_state.h"
#include "../runtime/builtins.h"

#endif