#ifndef EXECUTE_BYTECODE_H
#define EXECUTE_BYTECODE_H

#include "object_system/object.h"
#include "error/error.h"
#include "datatypes/stack.h"
#include "runtime/builtins.h"
#include "bytecode.h"
#include "macros.h"
#include "bytecode.h"
#include "object_system/object_operations.h"
#include "error/execution_state.h"

typedef struct {
    int* labels;
    int pointer;
    object scope;
    bytecode_program* program;
    stack object_stack;
    stack return_stack;
} bytecode_environment;

typedef struct {
    bool terminate;
    bytecode_program* program;
    object scope;
    int pointer;
} return_point;

void bytecode_enviroment_init(bytecode_environment* e);

void push(stack* stack, object o);
object execute_bytecode(bytecode_environment* environment);
void move_to_function(bytecode_environment* environment, function* f, bool termainate);

#endif