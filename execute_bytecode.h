#ifndef EXECUTE_BYTECODE_H
#define EXECUTE_BYTECODE_H

#include "object_system/object.h"
#include "error/error.h"
#include "datatypes/stack.h"
#include "bytecode.h"
#include "utility.h"
#include "bytecode.h"
#include "object_system/object_operations.h"
#include "options.h"

typedef struct {
    char* file;
    int line;
} Breakpoint;

typedef struct {
    vector breakpoints;
    bool running;
} debugger_state;

typedef struct {
    int pointer;
    Object scope;
    BytecodeProgram* executed_program;
    stack object_stack;
    stack return_stack;
    debugger_state debugger;
} BytecodeEnvironment;

typedef struct {
    bool terminate;
    BytecodeProgram* program;
    Object scope;
    int pointer;
} ReturnPoint;

void bytecode_environment_init(BytecodeEnvironment* e);
void bytecode_environment_free(BytecodeEnvironment* e);

void push(stack* stack, Object o);
Object pop(stack* stack);
Object execute_bytecode(Executor* E);
void move_to_function(Executor* E, Function* f, bool termainate);

#include "runtime/builtins.h"
#include "error/execution_state.h"

#endif