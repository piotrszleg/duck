#ifndef BYTECODE_OPTIMISATIONS_H
#define BYTECODE_OPTIMISATIONS_H

#include <stdbool.h>
#include "../bytecode.h"
#include "../options.h"

int path_length(const Instruction* code,  int path_start);
void optimise_bytecode(BytecodeProgram* prog, bool print_optimisations);

#endif