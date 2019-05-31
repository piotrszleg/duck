#ifndef BYTECODE_OPTIMISATIONS_H
#define BYTECODE_OPTIMISATIONS_H

#include <stdbool.h>
#include "../bytecode.h"
#include "../options.h"

int path_length(const instruction* code,  int path_start);
void optimise_bytecode(bytecode_program* prog);

#endif