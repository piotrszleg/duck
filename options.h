#ifndef OPTIONS_H
#define OPTIONS_H

extern bool g_ast_only;
extern bool g_print_ast;
extern bool g_print_ast_optimisations;
extern bool g_print_bytecode;
extern bool g_print_bytecode_optimisations;
extern bool g_optimise_ast;
extern bool g_optimise_bytecode;
extern bool g_debug_mode;

void handle_arguments(int argc, char **argv);

#endif