#ifndef PREDICT_INSTRUCTION_OUTPUT_H
#define PREDICT_INSTRUCTION_OUTPUT_H

#include "../bytecode/bytecode.h"
#include "../bytecode/bytecode_program.h"
#include "transformation.h"
#include "bytecode_optimisation_utility.h"

void predict_instruction_output(Executor* E, 
                                BytecodeProgram* program, 
                                Instruction* instruction, 
                                char* constants, 
                                unsigned* dummy_objects_counter, 
                                Transformation* transformation);
#endif