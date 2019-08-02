#ifndef OPTIONS_H
#define OPTIONS_H

typedef struct {
    bool ast_only;
    bool print_ast;
    bool print_ast_optimisations;
    bool print_bytecode;
    bool debug_mode;
    bool include_builtins;
    bool optimise_ast;
    bool ast_remove_useless_expressions;
    bool ast_conditionals_optimisation;
    bool ast_constants_folding;
    bool runtime_optimisations;
    bool optimise_bytecode;
    bool print_bytecode_optimisations;
    bool optimise_tail_calls;
    bool optimise_jump_to_return;
    bool optimise_stack_operations;
    bool constants_folding;
    bool remove_useless_operations;
    bool typed_variants;
} Options;

extern const Options default_options;

void handle_arguments(int argc, char **argv);

#endif