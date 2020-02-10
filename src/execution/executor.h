#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "../object_system/object.h"
#include "../bytecode/execute_bytecode.h"
#include "../ast/execute_ast.h"
#include "options.h"
#include "debugger.h"

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
    vector stack;

    Debugger debugger;
    BytecodeEnvironment bytecode_environment;
    Options options;
};

void executor_init(Executor* E);
void executor_deinit(Executor* E);
void executor_collect_garbage(Executor* E);
void executor_foreach_children(Executor* E, Executor* iterated_executor, 
                               ForeachChildrenCallback callback, void* data);

#endif