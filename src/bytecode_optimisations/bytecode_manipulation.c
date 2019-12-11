#include "bytecode_manipulation.h"

void remove_no_ops(Executor* E, vector* instructions, vector* informations, vector* transformations){
    int block_start=0;
    bool inside_block=false;
    for(int p=0; p<vector_count(instructions); p++){
        if(inside_block){
            if(vector_index_instruction(instructions, p)->type!=b_no_op || vector_index_instruction(instructions, p)->type==b_end){
                for(int i=block_start; i<=p-1; i++){
                    transformation_deinit(E, (Transformation*)vector_index(transformations, i));
                }
                vector_delete_range(instructions, block_start, p-1);
                vector_delete_range(informations, block_start, p-1);
                vector_delete_range(transformations, block_start, p-1);
                // move the pointer back by the number of instructions removed
                p=p-1-block_start;
                inside_block=false;
            }
        }
        if(vector_index_instruction(instructions, p)->type==b_no_op){
            block_start=p;
            inside_block=true;
        }
    }
}

void replace_dummies_in_transformations(BytecodeManipulation* manipulation, Dummy* to_replace, Dummy* replacement){
    unsigned instructions_count=count_instructions(vector_get_data(manipulation->instructions))+1;
    for(int p=0; p<instructions_count; p++){
        bool input_modified=false;
        Transformation* transformation=vector_index_transformation(manipulation->transformations, p);
        for(int i=0; i<transformation->inputs_count; i++){
            input_modified=input_modified||dummy_replace(manipulation->executor, &transformation->inputs[i], to_replace, replacement);
        }
        if(input_modified){
            Dummy** previous_outputs=malloc(sizeof(Dummy*)*transformation->outputs_count);
            for(int o=0; o<transformation->outputs_count; o++){
                previous_outputs[o]=transformation->outputs[o];
            }
            predict_instruction_output(manipulation->executor, 
                manipulation->program, 
                vector_index_instruction(manipulation->instructions, p), 
                vector_get_data(manipulation->constants), 
                manipulation->dummy_objects_counter, 
                transformation);
            for(int o=0; o<transformation->outputs_count; o++){
                if(previous_outputs[o]->type!=transformation->outputs[o]->type){
                    replace_dummies_in_transformations(manipulation, previous_outputs[o], transformation->outputs[o]);
                    heap_object_dereference(manipulation->executor, (HeapObject*)previous_outputs[o]);
                } else {
                    heap_object_dereference(manipulation->executor, (HeapObject*)transformation->outputs[o]);
                    transformation->outputs[o]=previous_outputs[o];
                }
            }
            free(previous_outputs);
        }
        for(int i=0; i<transformation->outputs_count; i++){
            dummy_replace(manipulation->executor, &transformation->outputs[i], to_replace, replacement);
        }
    }
}

void fill_with_no_op(BytecodeManipulation* manipulation, unsigned start, unsigned end) {
    if(manipulation->print_optimisations){
        highlight_instructions(vector_get_data(manipulation->instructions), vector_get_data(manipulation->constants), '-', start, end);
    }
    for(int i=start; i<=end; i++) {
        transformation_deinit(manipulation->executor, vector_index_transformation(manipulation->transformations, i));
        vector_index_instruction(manipulation->instructions, i)->type=b_no_op;
    }
}

// returns true if the result is inserting the instrcution and not changing b_no_op to it
bool insert_instruction(BytecodeManipulation* manipulation, unsigned index, Instruction* instruction, Transformation* transformation) {
    if(vector_index_instruction(manipulation->instructions, index)->type==b_no_op) {
        *vector_index_instruction(manipulation->instructions, index)=*instruction;
        *vector_index_transformation(manipulation->transformations, index)=*transformation;
        return false;
    } else {
        vector_insert(manipulation->instructions, index, instruction);
        vector_insert(manipulation->transformations, index, transformation);
        // copy information from previous instruction
        vector_insert(manipulation->informations, index, vector_index(manipulation->informations, index-1));
        return true;
    }
    if(manipulation->print_optimisations){
        highlight_instructions(vector_get_data(manipulation->instructions), vector_get_data(manipulation->constants), '+', index, index);
    }
}

bool insert_discard(BytecodeManipulation* manipulation, Dummy* discard_input, uint index) {
    Instruction discard_instruction={b_discard};
    Transformation discard_transformation;
    transformation_from_instruction(&discard_transformation, &discard_instruction);
    discard_transformation.inputs[0]=discard_input;
    heap_object_reference((HeapObject*)discard_transformation.inputs[0]);
    return insert_instruction(manipulation, index, &discard_instruction, &discard_transformation);
}

uint discard_transformation_inputs(BytecodeManipulation* manipulation, uint transformation_index){
    Transformation* producer=vector_index_transformation(manipulation->transformations, transformation_index);
    int to_discard=producer->inputs_count;
    uint instructions_written=0;
    for(int i=0; i<to_discard; i++){
        Instruction discard_instruction={b_discard};
        Transformation discard_transformation;
        transformation_from_instruction(&discard_transformation, &discard_instruction);
        discard_transformation.inputs[0]=producer->inputs[i];
        heap_object_reference((HeapObject*)discard_transformation.inputs[0]);
        if(insert_instruction(manipulation, transformation_index, &discard_instruction, &discard_transformation)) {
            instructions_written++;
            transformation_index++;
        }
    }
    return instructions_written;
}