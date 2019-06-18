#ifndef BYTECODE_OPTIMISATIONS_H
#define BYTECODE_OPTIMISATIONS_H

#include <stdbool.h>
#include "../bytecode.h"
#include "../options.h"
#include "../datatypes/vector.h"

int path_length(const Instruction* code,  int path_start);
void optimise_bytecode(Executor* E, BytecodeProgram* prog, bool print_optimisations);

#endif