#ifndef BYTECODE_OPTIMISATIONS_H
#define BYTECODE_OPTIMISATIONS_H

#include <stdbool.h>
#include "../bytecode/bytecode_program.h"
#include "../execution/execution.h"
#include "../containers/vector.h"
#include "../c_fixes.h"
#include "dummy.h"
#include "transformation.h"
#include "predict_instruction_output.h"
#include "bytecode_optimisation_utility.h"
#include "bytecode_manipulation.h"
#include "function_inlining.h"
#include "record_changes.h"

void optimise_bytecode(Executor* E, BytecodeProgram* prog, bool print_optimisations);

#endif