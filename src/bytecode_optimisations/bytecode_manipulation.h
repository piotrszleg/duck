#ifndef BYTECODE_MANIPULATION_H
#define BYTECODE_MANIPULATION_H

#include "../bytecode/bytecode.h"
#include "../bytecode/bytecode_program.h"
#include "../containers/vector.h"
#include "transformation.h"
#include "bytecode_optimisation_utility.h"
#include "predict_instruction_output.h"

typedef struct {
    BytecodeProgram* program;
    Executor* executor;
    vector* transformations;
    vector* instructions;
    vector* constants;
    vector* informations;
    bool print_optimisations;
    unsigned* dummy_objects_counter;
} BytecodeManipulation;


void remove_no_ops(Executor* E, vector* instructions, vector* informations, vector* transformations);
void replace_dummies_in_transformations(BytecodeManipulation* manipulation, Dummy* to_replace, Dummy* replacement);
void fill_with_no_op(BytecodeManipulation* manipulation, unsigned start, unsigned end);
// returns true if the result is inserting the instrcution and not changing b_no_op to it
bool insert_instruction(BytecodeManipulation* manipulation, unsigned index, Instruction* instruction, Transformation* transformation);
bool insert_discard(BytecodeManipulation* manipulation, Dummy* discard_input, uint index);
uint discard_transformation_inputs(BytecodeManipulation* manipulation, uint transformation_index);

#endif