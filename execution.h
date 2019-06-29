#ifndef EXECUTION_H
#define EXECUTION_H

#include <stdbool.h>
#include <string.h>
#include "bytecode.h"
#include "ast_to_bytecode.h"
#include "execute_bytecode.h"
#include "execute_ast.h"
#include "optimisations/ast_optimisations.h"
#include "optimisations/bytecode_optimisations.h"
#include "utility.h"
#include "object_system/object_operations.h"
#include "error/execution_state.h"
#include "parser/parser.h"
#include "options.h"
#include "macros.h"

Object evaluate(Executor* E, expression* parsing_result, Object scope, const char* file_name, bool delete_ast);
Object evaluate_string(Executor* E, const char* s, Object scope);
Object evaluate_file(Executor* E, const char* file_name, Object scope);
void execute_file(Executor* E, const char* file_name, char** arguments);
Object call_function(Executor* E, Function* f, Object* arguments, int arguments_count);
void executor_init(Executor* E);
void executor_deinit(Executor* E);

typedef struct {
    const char* file_name;
    unsigned line_number; 
} TracebackPoint;

#include "runtime/builtins.h"

typedef struct ASTExecutionState ASTExecutionState;
struct ASTExecutionState {
    bool initialized;
    bool returning;
    vector used_objects;
};

struct Executor {
    GarbageCollector* gc;
    unsigned line;
    unsigned column;
    vector traceback;
    const char* file;
    Object scope;

    Coroutine* coroutine;
    Object error;
    
    ASTExecutionState ast_execution_state;
    BytecodeEnvironment bytecode_environment;
    Options options;
};

#define INSERT_ERROR(type, cause, message, ...) \
    { Object err; \
    NEW_ERROR(err, type, cause, message, ##__VA_ARGS__) \
    if(E->error) { \
        dereference(E, E->error); \
    } \
    E->error=err; }

#define CHECK_FOR_ERROR \
    if(E->error.type!=t_null){ \
        Object error_to_return=E->error; \
        E->error=null_const; \
        return error_to_return; \
    }

#endif