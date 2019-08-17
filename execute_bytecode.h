#ifndef EXECUTE_BYTECODE_H
#define EXECUTE_BYTECODE_H

#include "object_system/object.h"
#include "error/error.h"
#include "datatypes/vector.h"
#include "bytecode_program.h"
#include "utility.h"
#include "bytecode_program.h"
#include "object_system/object_operations.h"
#include "options.h"
#include "c_fixes.h"

typedef struct {
    char* file;
    int line;
} Breakpoint;

typedef struct {
    vector breakpoints;
    bool running;
} Debugger;

typedef struct {
    int pointer;
    BytecodeProgram* executed_program;
    vector object_stack;
    vector return_stack;
    Debugger debugger;
} BytecodeEnvironment;

typedef struct {
    bool terminate;
    BytecodeProgram* program;
    Object scope;
    int pointer;
} ReturnPoint;

void bytecode_environment_init(BytecodeEnvironment* environment);
void bytecode_environment_deinit(BytecodeEnvironment* environment);

void push(vector* stack, Object o);
Object pop(vector* stack);
Object execute_bytecode(Executor* E);
void move_to_function(Executor* E, Function* f);
void create_return_point(Executor* E, bool terminate);
bool pop_return_point(Executor* E);
void create_variant(Executor* E, BytecodeProgram* program);

#include "runtime/builtins.h"
#include "error/execution_state.h"

#endif