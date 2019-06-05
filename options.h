#ifndef OPTIONS_H
#define OPTIONS_H

typedef struct {
    bool ast_only;
    bool print_ast;
    bool print_ast_optimisations;
    bool print_bytecode;
    bool print_bytecode_optimisations;
    bool optimise_ast;
    bool optimise_bytecode;
    bool debug_mode;
} options;

extern const options default_options;

void handle_arguments(int argc, char **argv);

#endif