#ifndef EXECUTE_BYTECODE_H
#define EXECUTE_BYTECODE_H

#include "../object_system/object.h"
#include "../error/error.h"
#include "../containers/vector.h"
#include "bytecode_program.h"
#include "../utility.h"
#include "bytecode_program.h"
#include "../object_system/object_operations.h"
#include "../execution/options.h"
#include "../c_fixes.h"

typedef struct {
    int pointer;
    BytecodeProgram* executed_program;
    vector return_stack;
} BytecodeEnvironment;

typedef struct {
    bool terminate;
    BytecodeProgram* program;
    Object scope;
    int pointer;
} ReturnPoint;

void bytecode_environment_init(BytecodeEnvironment* environment);
void bytecode_environment_deinit(BytecodeEnvironment* environment);

void print_object_stack(Executor* E, const vector* s);
void push(vector* stack, Object o);
Object pop(vector* stack);
Object execute_bytecode(Executor* E);
void move_to_function(Executor* E, Function* f);
void create_return_point(Executor* E, bool terminate);
bool pop_return_point(Executor* E);
void create_variant(Executor* E, BytecodeProgram* program);

#include "../runtime/builtins.h"
#include "../error/execution_state.h"

#endif