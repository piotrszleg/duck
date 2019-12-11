#ifndef BYTECODE_OPTIMISATION_UTILITY_H
#define BYTECODE_OPTIMISATION_UTILITY_H

#include "../bytecode/bytecode.h"
#include "../bytecode/bytecode_program.h"
#include "transformation.h"

unsigned count_instructions(Instruction* code);
void highlight_instructions(Instruction* instructions, void* constants, char symbol, int start, int end);
Assumption* get_argument_assumption(BytecodeProgram* program, unsigned index);
Assumption* get_upvalue_assumption(BytecodeProgram* program, Object identifier);
Dummy* assumption_to_dummy(Executor* E, Assumption* assumption, unsigned* dummy_objects_counter);
bool instruction_is_constant(Instruction* instruction, Transformation* transformation);
void print_transformations(Instruction* instructions, Transformation* transformations);

// true on success
bool constant_dummy_to_bytecode(Executor* E, 
                                Dummy* constant_dummy, 
                                unsigned position, 
                                vector* instructions, 
                                vector* transformations, 
                                vector* constants);



bool find_dummy_producer(vector* transformations, Dummy* dummy, int from, int* result);

#define VECTOR_INDEX_FUNCTION(type, postfix) \
    type* vector_index_##postfix(vector* v, int index);

VECTOR_INDEX_FUNCTION(Instruction, instruction)
VECTOR_INDEX_FUNCTION(Transformation, transformation)
VECTOR_INDEX_FUNCTION(InstructionInformation, information)

#undef VECTOR_INDEX_FUNCTION

#endif