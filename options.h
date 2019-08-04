#ifndef OPTIONS_H
#define OPTIONS_H

#include "utility.h"

#define OPTIONS \
    BOOLEAN(disable_bytecode, false) \
    BOOLEAN(print_ast, false) \
    BOOLEAN(print_ast_optimisations, false) \
    BOOLEAN(print_bytecode, false) \
    BOOLEAN(debug, false) \
    BOOLEAN(include_builtins, true) \
    BOOLEAN(optimise_ast, true) \
    BOOLEAN(ast_remove_useless_expressions, true) \
    BOOLEAN(ast_optimise_conditionals, true) \
    BOOLEAN(ast_fold_constants, true) \
    BOOLEAN(runtime_optimisations, true) \
    BOOLEAN(optimise_bytecode, true) \
    BOOLEAN(print_bytecode_optimisations, false) \
    BOOLEAN(optimise_tail_calls, true) \
    BOOLEAN(optimise_jump_to_return, true) \
    BOOLEAN(optimise_stack_operations, true) \
    BOOLEAN(fold_constants, true) \
    BOOLEAN(remove_useless_operations, true) \
    BOOLEAN(use_typed_instructions, true) \
    UNSIGNED(calls_before_optimisation, 3) \
    UNSIGNED(collected_calls, 10)

typedef struct {
    char* file_path;
    char** script_arguments;// NULL terminated array
    bool should_run;
    bool repl;
    #define BOOLEAN(name, default) bool name;
    #define UNSIGNED(name, default) unsigned name;
    OPTIONS
    #undef BOOLEAN
    #undef UNSIGNED
} Options;

extern const Options default_options;

void handle_arguments(int argc, char **argv);

#endif