#include "function_inlining.h"

bool is_subprogram(const BytecodeProgram* presumed_subprogram, const BytecodeProgram* program) {
    if(presumed_subprogram==program){
        return true;
    } else {
        for(int i=0; i<program->sub_programs_count; i++) {
            if(is_subprogram(presumed_subprogram, program->sub_programs[i])){
                return true;
            }
        }
        return false;
    }
}

// inserts transformations with any dummies as inputs and outputs
void insert_any_transformations(BytecodeManipulation* manipulation, unsigned position, unsigned count) {
    for(int i=position; i<position+count; i++){
        Transformation transformation;
        transformation_from_instruction(&transformation, vector_index_instruction(manipulation->instructions, i));
        for(int j=0; j<transformation.inputs_count; j++){
            transformation.inputs[j]=new_any_dummy(manipulation->executor);
            heap_object_reference((HeapObject*)transformation.inputs[j]);
        }
        for(int j=0; j<transformation.outputs_count; j++){
            transformation.outputs[j]=new_any_dummy(manipulation->executor);
            heap_object_reference((HeapObject*)transformation.outputs[j]);
        }
        vector_insert(manipulation->transformations, i, &transformation);
    }
}

void proccess_inlined_function_code(
    BytecodeManipulation* manipulation, 
    int start, 
    int end, 
    int inlined_function_constants_start, 
    int inlined_function_subprograms_start, 
    unsigned last_label_index, 
    Dummy* output){
    last_label_index++;
    int function_end_label=last_label_index;
    for(int j=start; j<end; j++){
        Instruction* instruction=vector_index_instruction(manipulation->instructions, j);
        switch (instruction->type)
        {
            case b_label:
            case b_jump:
                // shift argument, save the last label index to increment last_label index of the main program
                instruction->uint_argument+=last_label_index;
                break;
            case b_load_string:
                instruction->uint_argument+=inlined_function_constants_start;
                break;
            case b_function_2:
                instruction->uint_argument+=inlined_function_subprograms_start;
                break;
            case b_return: {
                Transformation* transformation=vector_index_transformation(manipulation->transformations, j);
                Dummy* return_input=transformation->outputs[0];
                // return_input will become input and output of jump instruction
                heap_object_reference((HeapObject*)return_input);
                heap_object_reference((HeapObject*)return_input);
                instruction->type=b_jump;
                instruction->uint_argument=function_end_label;
                // return transformation doesn't have an output
                // so we need to create new transformation for jump instruction
                transformation_deinit(manipulation->executor, transformation);
                transformation_init(transformation, 1, 1);
                transformation->inputs[0]=transformation->outputs[0]=return_input;
                break;
            }
            case b_tail_call: {
                instruction->type=b_call;
                Instruction jump_instruction={b_jump, .uint_argument=function_end_label};
                Transformation jump_transformation;
                transformation_init(&jump_transformation, 1, 1);
                jump_transformation.inputs[0]=jump_transformation.outputs[0]=vector_index_transformation(manipulation->transformations, j)->outputs[0];
                heap_object_reference((HeapObject*)jump_transformation.inputs[0]);
                heap_object_reference((HeapObject*)jump_transformation.outputs[0]);
                insert_instruction(manipulation, j+1, &jump_instruction, &jump_transformation);
                end++;
            }
            default:;
        }
    }
    Instruction label_instruction={b_label, .uint_argument=function_end_label};
    Transformation label_transformation;
    transformation_init(&label_transformation, 1, 1);
    // these dummies were referenced earlier
    label_transformation.outputs[0]=label_transformation.inputs[0]=output;
    insert_instruction(manipulation, end, &label_instruction, &label_transformation);
}

void inline_functions(
    Executor* E,
    BytecodeProgram* program,
    BytecodeManipulation* manipulation,
    vector* instructions, 
    vector* informations, 
    vector* transformations,
    vector* constants,
    bool print_optimisations) {
    unsigned last_label_index=0;
    for(int i=0; i<vector_count(instructions); i++){
        Instruction* instruction=vector_index_instruction(instructions, i);
        if(instruction->type==b_label){
            last_label_index=MAX(last_label_index, instruction->uint_argument);
        }
    }
    for(int i=0; i<vector_count(instructions); i++){
        if((vector_index_instruction(instructions, i)->type==b_call || vector_index_instruction(instructions, i)->type==b_tail_call)) {
            Dummy* input=vector_index_transformation(transformations, i)->inputs[0];
            if(input->type!=d_constant
                || input->constant_value.type!=t_function 
                || input->constant_value.fp->ftype!=f_bytecode){
                continue;// skip function if it isn't a bytecode function
            }
            Function* function=input->constant_value.fp;
            BytecodeProgram* function_program=(BytecodeProgram*)function->source_pointer;
            int provided_arguments=vector_index_transformation(transformations, i)->inputs_count-1;// -1 to skip the function object
            bool arguments_count_matches= provided_arguments==function->arguments_count
                                          || (function->variadic && provided_arguments>function->arguments_count);
            bool scope_matches= function->enclosing_scope.type==t_null 
                                || is_subprogram(function_program, program);
            // TODO: copy over subprograms of inlined function
            if(arguments_count_matches && scope_matches){
                Dummy* function_input=vector_index_transformation(transformations, i)->inputs[0];
                heap_object_reference((HeapObject*)function_input);
                Dummy* call_output=vector_index_transformation(transformations, i)->outputs[0];
                // call output is referenced as input and output to the label ending inlined function
                heap_object_reference((HeapObject*)call_output);
                heap_object_reference((HeapObject*)call_output);

                // remove call instruction and discard the function input
                fill_with_no_op(manipulation, i, i);
                Instruction discard_instruction={b_discard};
                Transformation discard_transformation;
                transformation_from_instruction(&discard_transformation, &discard_instruction);
                discard_transformation.inputs[0]=function_input;
                insert_instruction(manipulation, i, &discard_instruction, &discard_transformation);

                // insert instructions and informations from inlined function
                unsigned code_length=count_instructions(function_program->code);
                vector_insert_multiple(instructions, i+1, function_program->code, code_length);
                vector_insert_multiple(informations, i+1, function_program->information, code_length);
                // the inlined function isn't optimised in this optimisation session (it probably was optimised earlier anyways)
                // so all transformations of inlined function are just filled with any dummies
                // TODO: change these to proper transformations
                insert_any_transformations(manipulation, i+1, code_length);
                int inlined_function_constants_start=vector_count(constants);
                vector_insert_multiple(constants, inlined_function_constants_start, function_program->constants, function_program->constants_size);

                int inlined_function_subprograms_start=program->sub_programs_count;
                // merge subprograms of the inlined function with subrprograms of main function
                if(function_program->sub_programs_count!=0){
                    program->sub_programs_count+=function_program->sub_programs_count;
                    program->sub_programs=realloc(program->sub_programs, program->sub_programs_count*sizeof(BytecodeProgram*));
                    for(int i=0; i<function_program->sub_programs_count; i++) {
                        program->sub_programs[inlined_function_subprograms_start+i]=function_program->sub_programs[i];
                        heap_object_reference((HeapObject*)function_program->sub_programs[i]);
                    }
                }

                proccess_inlined_function_code(manipulation, i+1, i+1+code_length, 
                    inlined_function_constants_start, inlined_function_subprograms_start, last_label_index, call_output);

                if(print_optimisations){
                    printf("Inlining function:\n");
                    highlight_instructions(vector_get_data(instructions), vector_get_data(manipulation->constants), 
                        '+', i+1, i+code_length+1);
                }
                i+=code_length+1;
            }
        }
    }
}