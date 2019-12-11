#ifndef FUNCTION_INLINING_H
#define FUNCTION_INLINING_H

#include <stdbool.h>
#include "../bytecode/bytecode_program.h"
#include "bytecode_manipulation.h"

void inline_functions(
    Executor* E,
    BytecodeProgram* program,
    BytecodeManipulation* manipulation,
    vector* instructions, 
    vector* informations, 
    vector* transformations,
    vector* constants,
    bool print_optimisations);

#endif