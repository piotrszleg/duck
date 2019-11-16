#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "../object_system/object.h"
#include "../ast/execute_ast.h"
#include "options.h"

struct Executor {
    ObjectSystem object_system;
    unsigned line;
    unsigned column;
    vector traceback;
    const char* file;
    Object scope;

    Coroutine* coroutine;
    
    ASTExecutionState ast_execution_state;
    BytecodeEnvironment bytecode_environment;
    Options options;
};

void executor_init(Executor* E);
void executor_deinit(Executor* E);
void executor_collect_garbage(Executor* E);
void executor_foreach_children(Executor* E, Executor* iterated_executor, ManagedPointerForeachChildrenCallback callback);

#endif