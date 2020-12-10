#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "../object_system/object.h"
#include "../bytecode/execute_bytecode.h"
#include "../ast/execute_ast.h"
#include "options.h"
#include "debugger.h"
#include "../containers/stack.h"

typedef struct {
    char* file_name;
    unsigned line_number; 
} TracebackPoint;

struct Executor {
    ObjectSystem object_system;
    unsigned line;
    unsigned column;
    vector traceback;
    const char* file;
    Object scope;

    Coroutine* coroutine;
    
    bool returning;
    uint forward_allocated;
    Stack stack;

    Debugger debugger;
    BytecodeEnvironment bytecode_environment;
    Options options;

    Object undefined_argument;
};

void executor_init(Executor* E);
void executor_deinit(Executor* E);
void executor_collect_garbage(Executor* E);
void executor_foreach_children(Executor* E, Executor* iterated_executor, 
                               ForeachChildrenCallback callback, void* data);
void executor_stack_forward_allocate(Executor* E, uint count);
void executor_stack_forward_deallocate(Executor* E, uint count);
void executor_stack_push_allocated(Executor* E, Object object);
Object executor_stack_pop_allocated(Executor* E);
void executor_stack_push(Executor* E, Object object);
void executor_stack_remove(Executor* E, Object object);

#endif