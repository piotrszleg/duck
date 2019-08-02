#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include "dummy.h"
#include "../bytecode.h"

typedef struct
{
    bool visited;
    Dummy** inputs;
    int inputs_count;
    Dummy** outputs;
    int outputs_count;
} Transformation;

void transformation_init(Transformation* transformation, int inputs_count, int outputs_count);
void transformation_from_instruction(Transformation* transformation, Instruction* instruction);
void transformation_deinit(Executor* E, Transformation* transformation);

#endif