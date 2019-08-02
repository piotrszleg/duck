#include "transformation.h"

void transformation_init(Transformation* transformation, int inputs_count, int outputs_count){
    transformation->inputs_count=inputs_count;
    transformation->inputs=malloc(sizeof(Dummy*)*transformation->inputs_count);
    if(transformation->inputs_count>0){
        CHECK_ALLOCATION(transformation->inputs)
    }
    transformation->outputs_count=outputs_count;
    transformation->outputs=malloc(sizeof(Dummy*)*transformation->outputs_count);
    if(transformation->outputs_count>0){
        CHECK_ALLOCATION(transformation->outputs)
    }
}

void transformation_from_instruction(Transformation* transformation, Instruction* instruction){
    transformation_init(transformation, gets_from_stack(*instruction), pushes_to_stack(*instruction));
}

void transformation_deinit(Executor* E, Transformation* transformation){
    for(int i=0; i<transformation->inputs_count; i++){
        gc_object_dereference(E, (gc_Object*)transformation->inputs[i]);
    }
    transformation->inputs_count=0;
    for(int i=0; i<transformation->outputs_count; i++){
        gc_object_dereference(E, (gc_Object*)transformation->outputs[i]);
    }
    transformation->outputs_count=0;
    free(transformation->inputs);
    transformation->inputs=NULL;
    free(transformation->outputs);
    transformation->outputs=NULL;
}