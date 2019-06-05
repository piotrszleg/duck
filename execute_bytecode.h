#ifndef EXECUTE_BYTECODE_H
#define EXECUTE_BYTECODE_H

#include "object_system/object.h"
#include "error/error.h"
#include "datatypes/stack.h"
#include "runtime/builtins.h"
#include "bytecode.h"
#include "utility.h"
#include "bytecode.h"
#include "object_system/object_operations.h"
#include "error/execution_state.h"
#include "options.h"

typedef struct {
    vector breakpoints;// vector of execution_states
    bool running;
} debugger_state;

typedef struct {
    gc_pointer gcp;
    int pointer;
    object scope;
    bytecode_program* program;
    stack object_stack;
    stack return_stack;
    debugger_state debugger;
} bytecode_environment;

typedef struct {
    bool terminate;
    bytecode_program* program;
    object scope;
    int pointer;
} return_point;

void bytecode_environment_init(bytecode_environment* e);
void bytecode_environment_free(bytecode_environment* e);

void push(stack* stack, object o);
object execute_bytecode(executor* Ex, bytecode_environment* environment);
void move_to_function(executor* Ex, bytecode_environment* environment, function* f, bool termainate);

#endif