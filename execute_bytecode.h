#ifndef EXECUTE_BYTECODE_H
#define EXECUTE_BYTECODE_H

#include "object_system/object.h"
#include "error/error.h"
#include "datatypes/stack.h"
#include "builtins.h"
#include "bytecode.h"
#include "macros.h"
#include "bytecode.h"
#include "object_system/object_operations.h"

typedef struct {
    instruction* code;
    void* constants;
    int* labels;
    int pointer;
    int line;
    stack object_stack;
    stack return_stack;
} bytecode_environment;

void bytecode_enviroment_init(bytecode_environment* e);

void push(stack* stack, object o);
object execute_bytecode(bytecode_environment* environment, object scope);

#endif