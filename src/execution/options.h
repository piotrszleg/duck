#ifndef OPTIONS_H
#define OPTIONS_H

#include "../utility.h"

#define OPTIONS \
    BOOL(disable_garbage_collector, false) \
    BOOL(disable_bytecode, false) \
    BOOL(disable_ast_execution, false) \
    BOOL(print_ast, false) \
    BOOL(print_ast_optimisations, false) \
    BOOL(print_bytecode, false) \
    BOOL(debug, false) \
    BOOL(include_builtins, true) \
    BOOL(execute_prelude, true) \
    BOOL(optimise_ast, true) \
    BOOL(ast_remove_useless_expressions, true) \
    BOOL(ast_optimise_conditionals, true) \
    BOOL(ast_fold_constants, true) \
    BOOL(optimise_at_runtime, true) \
    BOOL(optimise_bytecode, true) \
    BOOL(print_bytecode_optimisations, false) \
    BOOL(optimise_tail_calls, true) \
    BOOL(optimise_jump_to_return, true) \
    BOOL(optimise_variable_lookup, true) \
    BOOL(inline_functions, true) \
    BOOL(inline_native_calls, true) \
    BOOL(fold_constants, true) \
    BOOL(remove_useless_operations, true) \
    BOOL(use_typed_instructions, true) \
    BOOL(compile_bytecode, true) \
    UINT(calls_before_optimisation, 5) \
    UINT(collected_calls, 10) \
    UINT(constant_threshold, 3)

typedef struct {
    char* file_path;
    char** script_arguments;// NULL terminated array
    bool should_run;
    bool repl;
    #define BOOL(name, default) bool name;
    #define UINT(name, default) unsigned name;
    OPTIONS
    #undef BOOL
    #undef UINT
} Options;

extern const Options default_options;

void handle_arguments(int argc, char **argv);

#endif