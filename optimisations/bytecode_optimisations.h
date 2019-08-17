#ifndef BYTECODE_OPTIMISATIONS_H
#define BYTECODE_OPTIMISATIONS_H

#include <stdbool.h>
#include "../bytecode_program.h"
#include "../execution.h"
#include "../datatypes/vector.h"
#include "../c_fixes.h"
#include "dummy.h"
#include "transformation.h"

void optimise_bytecode(Executor* E, BytecodeProgram* prog, bool print_optimisations);

#endif