#ifndef EXECUTION_H
#define EXECUTION_H

#include <stdbool.h>
#include <string.h>
#include "bytecode.h"
#include "ast_to_bytecode.h"
#include "execute_bytecode.h"
#include "optimisations/ast_optimisations.h"
#include "optimisations/bytecode_optimisations.h"
#include "runtime/builtins.h"
#include "utility.h"
#include "object_system/object_operations.h"
#include "error/execution_state.h"
#include "parser/parser.h"
#include "execute_ast.h"
#include "execute_ast.h"
#include "options.h"
#include "macros.h"

Object evaluate_string(Executor* E, const char* s, Object scope);
Object evaluate_file(Executor* E, const char* file_name, Object scope);
void execute_file(Executor* E, const char* file_name);
Object call_function(Executor* E, Function* f, Object* arguments, int arguments_count);

struct Executor {
    GarbageCollector* gc;
    unsigned line;
    unsigned column;
    unsigned* traceback;
    const char* file;

    Coroutine* coroutine;

    bool ast_returning;
    BytecodeEnvironment bytecode_environment;
    Options options;
};

#endif