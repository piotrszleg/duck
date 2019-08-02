#include "bytecode_optimisations.h"

#define LOG_IF_ENABLED(message, ...) \
    if(print_optimisations){ \
        printf(message, ##__VA_ARGS__); \
    }

int count_instructions(Instruction* code){
    int p=0;
    for(; code[p].type!=b_end; p++);
    return p;
}

void highlight_instructions(Instruction* instructions, void* constants, char symbol, int start, int end){
    int pointer=0;
    while(instructions[pointer].type!=b_end){
        if(pointer>=start && pointer<=end){
            printf("%c ", symbol);
        } else {
            printf("  ");
        }
        print_instruction(instructions[pointer], constants);
        printf("\n");
        pointer++;
    }
}

static bool operand_has_side_effects(Dummy* dummy){
    return !dummy_is_typed(dummy)
        || (dummy_type(dummy)==t_table && dummy_type(dummy)==t_function);
}

bool has_side_effects(Instruction* instruction, Transformation* transformation) {
    switch(instruction->type) {
        case b_null:
        case b_load_int:
        case b_load_float:
        case b_load_string:
        case b_table_literal:
        case b_function_1:
        case b_function_2:
        case b_double:
            return false;
        case b_add:
        case b_subtract:
        case b_multiply:
        case b_divide:
        case b_divide_floor:
        case b_modulo:
        case b_add_int:
        case b_subtract_int:
        case b_multiply_int:
        case b_divide_int:
        case b_divide_floor_int:
        case b_modulo_int:
        case b_add_float:
        case b_subtract_float:
        case b_multiply_float:
        case b_divide_float:
        case b_add_string:
            return operand_has_side_effects(transformation->inputs[0]) 
            ||     operand_has_side_effects(transformation->inputs[1]);
        case b_binary:
            return operand_has_side_effects(transformation->inputs[1]) 
            ||     operand_has_side_effects(transformation->inputs[2]);
        case b_prefix:
            return operand_has_side_effects(transformation->inputs[1]);
        case b_minus:
        case b_minus_int:  
        case b_minus_float:
        case b_not:
            return operand_has_side_effects(transformation->inputs[0]);
        default: return true;
    }
}

Assumption* get_argument_assumption(BytecodeProgram* program, unsigned index){
    if(program->assumptions==NULL){
        return NULL;
    } else {
        return &program->assumptions[index];
    }
}

Assumption* get_upvalue_assumption(BytecodeProgram* program, Object identifier) {
    if(identifier.type!=t_string||program->assumptions==NULL){
        return NULL;
    }
    for(int i=0; i<program->upvalues_count; i++){
        char* upvalue_name=program->constants+program->upvalues[i];
        if(strcmp(upvalue_name, identifier.text)==0){
            return &program->assumptions[program->expected_arguments+i];
        }
    }
    return NULL;
}

Dummy* assumption_to_dummy(Executor* E, Assumption* assumption, unsigned* dummy_objects_counter){
    if(assumption==NULL){
        return new_any_type_dummy(E, dummy_objects_counter);
    }
    switch(assumption->assumption_type){
        case a_constant:
            return new_constant_dummy(E, assumption->constant, dummy_objects_counter);
        case a_type:
            return new_known_type_dummy(E, assumption->type, dummy_objects_counter); 
        default:
            THROW_ERROR(BYTECODE_ERROR, "Incorrect assumption type.");
            return NULL;
    }
}

// function writes to transformation outputs the result of evaluating instruction with it's input
void predict_instruction_output(Executor* E, BytecodeProgram* program, Instruction* instr, char* constants, unsigned* dummy_objects_counter, Transformation* transformation){
    Dummy** outputs=transformation->outputs;
    Dummy** inputs=transformation->inputs;
    if(carries_stack(instr->type)){
        int i=0;
        // jump_not takes one item from the stack as a predicate
        // so it needs to be skipped
        if(instr->type==b_jump_not){
            i++;
        }
        for(; i<transformation->inputs_count; i++){
            outputs[transformation->inputs_count-1-i]=inputs[i];
        }
        return;
    }
    switch (instr->type){
        case b_null:
            outputs[0]=new_constant_dummy(E, null_const, dummy_objects_counter);
            return;
        case b_load_int:
            outputs[0]=new_constant_dummy(E, to_int(instr->int_argument), dummy_objects_counter);
            return;
        case b_load_float:
            outputs[0]=new_constant_dummy(E, to_float(instr->float_argument), dummy_objects_counter);
            return;
        case b_load_string:
            outputs[0]=new_constant_dummy(E, to_string(constants+instr->uint_argument), dummy_objects_counter);
            return;
        case b_table_literal:
            outputs[0]=new_known_type_dummy(E, t_table, dummy_objects_counter);
            return;
        case b_function_1:
            outputs[0]=new_known_type_dummy(E, t_function, dummy_objects_counter);
            return;
        case b_function_2:
            outputs[0]=inputs[0];
            return;
        case b_double:
            outputs[0]=inputs[0];
            outputs[1]=inputs[0];
            return;
        case b_push_to_top:
            outputs[instr->uint_argument]=inputs[0];
            outputs[0]=inputs[instr->uint_argument];
            for(int i=1; i<instr->uint_argument-1; i++){
                outputs[i]=outputs[instr->uint_argument-2-i];
            }
            return;
        case b_swap:
        {
            for(int i=0; i<transformation->outputs_count; i++){
                outputs[transformation->outputs_count-1-i]=inputs[i];
            }
            int left=transformation->outputs_count-1-instr->swap_argument.left;
            int right=transformation->outputs_count-1-instr->swap_argument.right;
            Dummy* temp=outputs[left];
            outputs[left]=outputs[right];
            outputs[right]=temp;
            return;
        }
        case b_get:
            if(inputs[0]->type==d_constant){
                outputs[0]=assumption_to_dummy(E, get_upvalue_assumption(program, inputs[0]->constant_value), dummy_objects_counter);
                return;
            }
            break;
        case b_set:
            outputs[0]=inputs[1];
            return;
        case b_binary:
        {
            if(inputs[0]->type==d_constant&&!operand_has_side_effects(inputs[1]) && !operand_has_side_effects(inputs[2])
            ){
                if(inputs[0]->type==d_constant && inputs[1]->type==d_constant && inputs[2]->type==d_constant){
                    Object operator_result=operator(E, inputs[1]->constant_value, inputs[2]->constant_value, inputs[0]->constant_value.text);
                    if(is_unhandled_error(E, operator_result)){
                        set(E, operator_result, to_string("handled"), to_int(1));
                        dereference(E, &operator_result);
                        outputs[0]=new_any_type_dummy(E, dummy_objects_counter);
                    } else {
                        outputs[0]=new_constant_dummy(E, operator_result, dummy_objects_counter);
                    }
                } else {
                    outputs[0]=new_known_type_dummy(E, dummy_type(inputs[1]), dummy_objects_counter);
                }
                return;
            }
            break;
        }
        #define BINARY_OPERATOR(instruction, op) \
            case instruction: \
            { \
                if(!operand_has_side_effects(inputs[0]) && !operand_has_side_effects(inputs[1]) \
                ){ \
                    if(inputs[0]->type==d_constant && inputs[1]->type==d_constant){ \
                        Object operator_result=operator(E, inputs[0]->constant_value, inputs[1]->constant_value, op); \
                        if(is_unhandled_error(E, operator_result)){ \
                            set(E, operator_result, to_string("handled"), to_int(1)); \
                            dereference(E, &operator_result); \
                            outputs[0]=new_any_type_dummy(E, dummy_objects_counter); \
                        } else { \
                            outputs[0]=new_constant_dummy(E, operator_result, dummy_objects_counter); \
                        } \
                    } else { \
                        outputs[0]=new_known_type_dummy(E, dummy_type(inputs[0]), dummy_objects_counter); \
                    } \
                    return; \
                } \
                break; \
            }
        BINARY_OPERATOR(b_add, "+")
        BINARY_OPERATOR(b_subtract, "-")
        BINARY_OPERATOR(b_multiply, "*")
        BINARY_OPERATOR(b_divide, "/")
        BINARY_OPERATOR(b_divide_floor, "//")
        BINARY_OPERATOR(b_modulo, "%")
        BINARY_OPERATOR(b_add_int, "+")
        BINARY_OPERATOR(b_subtract_int, "-")
        BINARY_OPERATOR(b_multiply_int, "*")
        BINARY_OPERATOR(b_divide_int, "/")
        BINARY_OPERATOR(b_divide_floor_int, "//")
        BINARY_OPERATOR(b_modulo_int, "%")
        BINARY_OPERATOR(b_add_float, "+")
        BINARY_OPERATOR(b_subtract_float, "-")
        BINARY_OPERATOR(b_multiply_float, "*")
        BINARY_OPERATOR(b_divide_float, "/")
        BINARY_OPERATOR(b_add_string, "+")
        #undef BINARY_OPERATOR
        #define PREFIX_OPERATOR(instruction, op) \
            case instruction: \
            { \
                if(!operand_has_side_effects(inputs[0])){ \
                    if(inputs[0]->type==d_constant){ \
                        outputs[0]=new_constant_dummy(E, operator(E, inputs[0]->constant_value, null_const, op), dummy_objects_counter); \
                    } else { \
                        outputs[0]=new_known_type_dummy(E, dummy_type(inputs[0]), dummy_objects_counter); \
                    } \
                    return; \
                } \
                break; \
            }
        PREFIX_OPERATOR(b_minus, "-")
        PREFIX_OPERATOR(b_minus_int, "-")
        PREFIX_OPERATOR(b_minus_float, "-")
        PREFIX_OPERATOR(b_not, "!")
        #undef PREFIX_OPERATOR
        default:;
    }
    for(int i=0; i<transformation->outputs_count; i++){
        outputs[i]=new_any_type_dummy(E, dummy_objects_counter);
    }
}

static void print_transformations(Instruction* instructions, Transformation* transformations){
    printf("Instructions transformations:\n");
    unsigned instructions_count=count_instructions(instructions)+1;
    for(int p=0; p<instructions_count; p++){
        printf("%s (", INSTRUCTION_NAMES[instructions[p].type]);
        for(int i=0; i<transformations[p].inputs_count; i++){
            if(i!=0){
                printf(", ");
            }
            dummy_print(transformations[p].inputs[i]);
        }
        printf(")");
        printf("->(");
        for(int i=0; i<transformations[p].outputs_count; i++){
            if(i!=0){
                printf(", ");
            }
            dummy_print(transformations[p].outputs[i]);
        }
        printf(")\n");
    }
}

// true on success
bool constant_dummy_to_bytecode(Executor* E, Dummy* constant_dummy, unsigned position, vector* instructions, vector* transformations, vector* constants){
    Object object=constant_dummy->constant_value;
    #define LITERAL(type, argument) \
        { \
            Instruction replacement={type, argument}; \
            vector_insert(instructions, position, &replacement); \
        }
    switch(object.type){
        case t_null:
            LITERAL(b_null,)
            break;
        case t_float:
            LITERAL(b_load_float, .float_argument=object.float_value);
            break;
        case t_int:
            LITERAL(b_load_int, .int_argument=object.int_value);
            break;
        case t_string:{
            unsigned push_position=vector_count(constants);
            vector_insert_multiple(constants, push_position, object.text, strlen(object.text)+1);
            LITERAL(b_load_string, .uint_argument=push_position);
            break;
        }
        default: return false;
    }
    #undef LITERAL
    Transformation transformation;
    transformation_init(&transformation, 0, 1);
    transformation.outputs[0]=constant_dummy;
    gc_object_reference((gc_Object*)transformation.outputs[0]);
    vector_insert(transformations, position, &transformation);
    return true;
}

#define VECTOR_INDEX_FUNCTION(type, postfix) \
    type* vector_index_##postfix(vector* v, int index) { \
        return (type*)vector_index(v, index); \
    }

VECTOR_INDEX_FUNCTION(Instruction, instruction)
VECTOR_INDEX_FUNCTION(Transformation, transformation)
VECTOR_INDEX_FUNCTION(InstructionInformation, information)

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
            predict_instruction_output(manipulation->executor, manipulation->program, vector_index_instruction(manipulation->instructions, p), 
                vector_get_data(manipulation->constants), manipulation->dummy_objects_counter, transformation);
            for(int o=0; o<transformation->outputs_count; o++){
                if(previous_outputs[o]->type!=transformation->outputs[o]->type){
                    gc_object_reference((gc_Object*)transformation->outputs[o]);
                    replace_dummies_in_transformations(manipulation, previous_outputs[o], transformation->outputs[o]);
                    gc_object_dereference(manipulation->executor, (gc_Object*)previous_outputs[o]);
                } else {
                    gc_object_dereference(manipulation->executor, (gc_Object*)transformation->outputs[o]);
                    transformation->outputs[o]=previous_outputs[o];
                    gc_object_reference((gc_Object*)transformation->outputs[o]);
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

void insert_instruction(BytecodeManipulation* manipulation, unsigned index, Instruction* instruction, Transformation* transformation) {
    if(vector_index_instruction(manipulation->instructions, index)->type==b_no_op) {
        *vector_index_instruction(manipulation->instructions, index)=*instruction;
        *vector_index_transformation(manipulation->transformations, index)=*transformation;
    } else {
        vector_insert(manipulation->instructions, index, instruction);
        vector_insert(manipulation->transformations, index, transformation);
        // copy information from previous instruction
        vector_insert(manipulation->informations, index, vector_index(manipulation->informations, index));
    }
    if(manipulation->print_optimisations){
        highlight_instructions(vector_get_data(manipulation->instructions), vector_get_data(manipulation->constants), '+', index, index);
    }
}

void stack_usage_optimisations(
    Executor* E, 
    BytecodeProgram* program, 
    BytecodeManipulation* manipulation,
    vector* instructions, 
    vector* transformations, 
    vector* constants, 
    bool print_optimisations) {
    for(int pointer=count_instructions((Instruction*)vector_get_data(instructions))-1; pointer>=1; pointer--){
        // change calls to tail calls if possible
        // replace variable lookup with stack operations
        if(vector_index_instruction(instructions, pointer)->type==b_set && vector_index_instruction(instructions, pointer+1)->type==b_discard
           && !vector_index_instruction(instructions, pointer)->bool_argument /* argument tells whether the variable is used in closure, we can't tell if the closure changes the variable*/
           && (vector_index_instruction(instructions, pointer-1)->type==b_load_string || vector_index_instruction(instructions, pointer-1)->type==b_load_int)){// don't optimise nested paths like table.key, only single name paths
            if(print_optimisations){
                printf("Found a set Instruction\n");
                highlight_instructions(vector_get_data(instructions), vector_get_data(manipulation->constants), 
                    '>', pointer-1, pointer);
            }
            bool first_get_removal=true;
            bool used=false;
            BytecodeIterator progress_state;
            int get_search;
            BYTECODE_FOR(progress_state, get_search, vector_get_data(instructions)){
                if(changes_scope(vector_index_instruction(instructions, get_search)->type)){
                    // we optimised all gets in this scope so the variable isn't needed anymore
                    break;
                }
                if(vector_index_instruction(instructions, get_search)->type==b_get 
                && instructions_equal(*vector_index_instruction(instructions, get_search-1), *vector_index_instruction(instructions, pointer-1), vector_get_data(constants))){
                    if(print_optimisations){
                        printf("Found a corresponding get Instruction\n");
                        highlight_instructions(vector_get_data(instructions), vector_get_data(manipulation->constants), 
                            '>', get_search-1, get_search);
                    }
                    if(first_get_removal){
                        LOG_IF_ENABLED("Removing discard Instruction after set Instruction:\n");
                        fill_with_no_op(manipulation, pointer+1, pointer+1);
                        first_get_removal=false;
                    } else{
                        Instruction double_instruction={b_double};
                        Transformation double_transformation;
                        transformation_from_instruction(&double_transformation, &double_instruction);
                        Dummy* doubled=((Transformation*)vector_index(transformations, pointer))->outputs[0];
                        double_transformation.inputs[0]=doubled;
                        double_transformation.outputs[0]=double_transformation.outputs[1]=doubled;
                        gc_object_reference((gc_Object*)doubled);
                        gc_object_reference((gc_Object*)doubled);
                        gc_object_reference((gc_Object*)doubled);
                        insert_instruction(manipulation, pointer+1, &double_instruction, &double_transformation);
                        get_search=bytecode_iterator_next(&progress_state, vector_get_data(instructions));// skip inserted instruction
                    }
                    // search for references to dummy object and replace them with this one
                    Dummy* to_replace=vector_index_transformation(transformations, get_search)->outputs[0];
                    gc_object_reference((gc_Object*)to_replace);
                    fill_with_no_op(manipulation, get_search-1, get_search);
                    replace_dummies_in_transformations(manipulation, to_replace, vector_index_transformation(transformations, pointer)->outputs[0]);
                    gc_object_dereference(E, (gc_Object*)to_replace);

                    if(get_search>=vector_count(instructions)){
                        break;// after removing the path get_search points to b_end, if loop continued from this point it would get past the code's end
                    }
                }
            }
            if(!used){
                // the variable isn't used in it's own scope and in any closure, so it can be removed
                LOG_IF_ENABLED("Removing set Instruction:\n");
                Dummy* to_replace=vector_index_transformation(transformations, pointer)->outputs[0];
                gc_object_reference((gc_Object*)to_replace);
                Dummy* replacing=vector_index_transformation(transformations, pointer)->inputs[1];
                gc_object_reference((gc_Object*)replacing);
                fill_with_no_op(manipulation, pointer-1, pointer);
                replace_dummies_in_transformations(manipulation, to_replace, replacing);
                gc_object_dereference(E, (gc_Object*)to_replace);
                gc_object_dereference(E, (gc_Object*)replacing);
            }
        }
    }
}

// replace jump to return or end instruction with return
// this allows more tail call optimisations to happen
void jump_to_return(
    BytecodeManipulation* manipulation,
    vector* instructions, 
    vector* transformations){
    for(int pointer=0; vector_index_instruction(instructions, pointer)->type!=b_end; pointer++){
        if(vector_index_instruction(instructions, pointer)->type==b_jump && vector_index_transformation(transformations, pointer)->inputs_count==1){
            int jump_destination=find_label(vector_get_data(instructions), vector_index_instruction(instructions, pointer)->uint_argument);
            int instruction_after_label=jump_destination+1;
            // skip following labels and no-ops
            while(vector_index_instruction(instructions, instruction_after_label)->type==b_label 
                || vector_index_instruction(instructions, instruction_after_label)->type==b_no_op){
                instruction_after_label++;
            }
            if(vector_index_instruction(instructions, instruction_after_label)->type==b_return 
                || vector_index_instruction(instructions, instruction_after_label)->type==b_end){
                
                // remember what was the input to jump instruction
                Dummy* to_return=vector_index_transformation(transformations, pointer)->inputs[0];
                gc_object_reference((gc_Object*)to_return);
                // remove destination label
                fill_with_no_op(manipulation, jump_destination, jump_destination);

                // change jump instruction to return
                transformation_deinit(manipulation->executor, vector_index_transformation(transformations, pointer));
                vector_index_instruction(instructions, pointer)->type=b_return;
                transformation_from_instruction(vector_index_transformation(transformations, pointer), vector_index_instruction(instructions, pointer));
                vector_index_transformation(transformations, pointer)->inputs[0]=to_return;
            }
            break;
        }
    }
}

// replace normal calls with tail calls if possible
bool tail_calls(BytecodeManipulation* manipulation, vector* instructions, int pointer, bool print_optimisations){
    if(vector_index_instruction(instructions, pointer)->type==b_call){
        if(vector_index_instruction(instructions, pointer+1)->type==b_end){
            vector_index_instruction(instructions, pointer)->type=b_tail_call;
            return true;
        }
        if(vector_index_instruction(instructions, pointer+1)->type==b_return){
            vector_index_instruction(instructions, pointer)->type=b_tail_call;
            fill_with_no_op(manipulation, pointer+1, pointer+1);
            return true;
        }
    }
    return false;
}

// if an operation has no side effects and it's result is immediately discarded removes it
bool remove_useless_operations(BytecodeManipulation* manipulation, vector* instructions, vector* transformations, int pointer, bool print_optimisations){
    if(vector_index_instruction(instructions, pointer)->type==b_discard){
        Dummy* discard_input=vector_index_transformation(transformations, pointer)->inputs[0];
        // search for an instruction that outputs the discarded object
        for(int search=pointer-1; search>=0; search--){
            for(int o=0; o<vector_index_transformation(transformations, search)->outputs_count; o++){
                if(dummies_equal(vector_index_transformation(transformations, search)->outputs[o], discard_input)
                    &&( vector_index_transformation(transformations, search)->outputs_count==1 || vector_index_instruction(instructions, search)->type==b_double)
                    && !has_side_effects(vector_index_instruction(instructions, search), vector_index_transformation(transformations, search))
                    ){
                    LOG_IF_ENABLED("Removing operation which result is immediately discarded:\n")
                    Transformation* producer=vector_index_transformation(transformations, search);
                    // discard inputs to producer
                    int to_discard=producer->inputs_count;
                    if(vector_index_instruction(instructions, search)->type==b_double){
                        to_discard--;
                    }
                    for(int i=0; i<to_discard; i++){
                        Instruction discard_instruction={b_discard};
                        Transformation discard_transformation;
                        transformation_from_instruction(&discard_transformation, &discard_instruction);
                        discard_transformation.inputs[0]=producer->inputs[i];
                        gc_object_reference((gc_Object*)discard_transformation.inputs[0]);
                        insert_instruction(manipulation, search, &discard_instruction, &discard_transformation);
                        search++;
                        pointer++;
                    }
                    // remove producer and discard instruction
                    fill_with_no_op(manipulation, search, search);
                    fill_with_no_op(manipulation, pointer, pointer);
                    return true;
                }
            }
        }
    }
    return false;
}

// if operation has no side effect and it's output is known it is replaced with it's result literal
bool constants_folding(
    Executor* E,
    BytecodeManipulation* manipulation,
    vector* instructions, 
    vector* transformations, 
    vector* constants, 
    int* pointer, 
    bool print_optimisations
){
    if(vector_index_transformation(transformations, *pointer)->outputs_count==1 
    && vector_index_transformation(transformations, *pointer)->outputs[0]->type==d_constant
    && !instruction_is_literal(vector_index_instruction(instructions, *pointer)->type)
    && !has_side_effects(vector_index_instruction(instructions, *pointer), vector_index_transformation(transformations, *pointer))){
        Transformation producer=*vector_index_transformation(transformations, *pointer);
        if(constant_dummy_to_bytecode(E, vector_index_transformation(transformations, *pointer)->outputs[0], *pointer+1, instructions, transformations, constants)){
            LOG_IF_ENABLED("Replacing operation with it's result:\n")
            // discard inputs to producer
            for(int i=0; i<producer.inputs_count; i++){
                Instruction discard_instruction={b_discard};
                Transformation discard_transformation;
                transformation_from_instruction(&discard_transformation, &discard_instruction);
                discard_transformation.inputs[0]=producer.inputs[i];
                gc_object_reference((gc_Object*)discard_transformation.inputs[0]);
                insert_instruction(manipulation, (*pointer)+1, &discard_instruction, &discard_transformation);
            }
            fill_with_no_op(manipulation, *pointer, *pointer);
            (*pointer)+=producer.inputs_count+1;
            return true;
        }
    }
    return false;
}

bool typed_variants(vector* instructions, vector* transformations, int pointer){
    #define OPERATOR_VARIANT(base_instruction, type_name) \
    if(vector_index_instruction(instructions, pointer)->type==base_instruction \
        && dummy_type(vector_index_transformation(transformations, pointer)->inputs[0])==t_##type_name \
        && dummy_type(vector_index_transformation(transformations, pointer)->inputs[1])==t_##type_name){ \
            vector_index_instruction(instructions, pointer)->type=base_instruction##_##type_name; \
            return true; \
        }
    OPERATOR_VARIANT(b_add, int)
    OPERATOR_VARIANT(b_add, float)
    OPERATOR_VARIANT(b_add, string)
    OPERATOR_VARIANT(b_subtract, int)
    OPERATOR_VARIANT(b_subtract, float)
    OPERATOR_VARIANT(b_multiply, int)
    OPERATOR_VARIANT(b_multiply, float)
    OPERATOR_VARIANT(b_divide, int)
    OPERATOR_VARIANT(b_divide, float)
    OPERATOR_VARIANT(b_divide_floor, int)
    OPERATOR_VARIANT(b_modulo, int)
    #define PREFIX_VARIANT(base_instruction, type_name) \
        if(vector_index_instruction(instructions, pointer)->type==base_instruction \
        && dummy_type(vector_index_transformation(transformations, pointer)->inputs[0])==t_##type_name){ \
            vector_index_instruction(instructions, pointer)->type==base_instruction##_##type_name; \
            return true; \
        }
    OPERATOR_VARIANT(b_minus, int)
    OPERATOR_VARIANT(b_minus, float)
    return false;
}

void optimise_bytecode(Executor* E, BytecodeProgram* program, bool print_optimisations){
    for(int i=0; i<program->sub_programs_count; i++){
        optimise_bytecode(E, &program->sub_programs[i], print_optimisations);
    }
    if(print_optimisations){
        print_bytecode_program(program);
    }

    // step 1: deduce how the objects will flow from one instruction to the next
    // also get a list of externally provided objects

    int instructions_count=count_instructions(program->code)+1;
    vector provided, stack, transformations, instructions, informations, constants;
    vector_init(&provided, sizeof(Dummy*), 64);
    vector_init(&stack, sizeof(Dummy*), 128);
    vector_init(&transformations, sizeof(Transformation), instructions_count);
    vector_extend(&transformations, instructions_count);
    vector_from(&instructions, sizeof(Instruction), program->code, instructions_count);
    vector_from(&informations, sizeof(InstructionInformation), program->information, instructions_count);
    vector_from(&constants, sizeof(char), program->constants, program->constants_size);
    unsigned int dummy_objects_counter=0;

    BytecodeIterator progress_state;
    for(int p=bytecode_iterator_start(&progress_state, program->code, 0); p!=-1; p=bytecode_iterator_next(&progress_state, program->code)){
        if(p==0){
            for(int i=vector_count(&provided)-1; i>=0; i--){
                vector_push(&stack, vector_index(&provided, i));
            }
        }
        if(!vector_index_transformation(&transformations, p)->visited){
            Transformation transformation;
            if(carries_stack(program->code[p].type)){
                // jump_not takes one item from the stack as a predicate
                if(program->code[p].type==b_jump_not){
                    transformation_init(&transformation, vector_count(&stack), vector_count(&stack)-1);
                } else {
                    transformation_init(&transformation, vector_count(&stack), vector_count(&stack));
                }
            } else {
                transformation_from_instruction(&transformation, &program->code[p]);
            }
            
            for(int i=0; i<transformation.inputs_count; i++){
                if(!vector_empty(&stack)){
                    transformation.inputs[i]=*(Dummy**)vector_pop(&stack);
                } else {
                    Dummy* dummy=assumption_to_dummy(E, get_argument_assumption(program, vector_count(&provided)), &dummy_objects_counter);
                    vector_push(&provided, &dummy);
                    transformation.inputs[i]=dummy;
                    // in this case dummy is also referenced by provided
                    gc_object_reference((gc_Object*)transformation.inputs[i]);
                }
                // input is referenced by the transformation
                gc_object_reference((gc_Object*)transformation.inputs[i]);
            }
            predict_instruction_output(E, program, &program->code[p], (char*)program->constants, &dummy_objects_counter, &transformation);
            for(int i=0; i<transformation.outputs_count; i++){
                vector_push(&stack, &transformation.outputs[i]);
                // output is referenced by the transformation
                gc_object_reference((gc_Object*)transformation.outputs[i]);
            }
            *vector_index_transformation(&transformations, p)=transformation;
            vector_index_transformation(&transformations, p)->visited=true;
        } else {
            bool inputs_changed=false;
            for(int i=0; i<vector_index_transformation(&transformations, p)->inputs_count; i++){
                Dummy* from_stack=*(Dummy**)vector_pop(&stack);
                Dummy* expected=(Dummy*)vector_index_transformation(&transformations, p)->inputs[i];
                if(!dummies_equal(expected, from_stack)){
                    if(dummy_contains(from_stack, expected)){
                        gc_object_dereference(E, (gc_Object*)expected);
                        gc_object_reference((gc_Object*)from_stack);
                        vector_index_transformation(&transformations, p)->inputs[i]=from_stack;
                        inputs_changed=true;
                    } else if(!dummy_contains(expected, from_stack)){
                        Dummy* or_dummy=new_or_dummy(E, expected, from_stack, &dummy_objects_counter);
                        // expected is already referenced by the instruction
                        gc_object_reference((gc_Object*)or_dummy);// or_dummy referenced by transformation
                        gc_object_dereference(E, (gc_Object*)expected);// expected is no longer referenced by transformation itself
                        vector_index_transformation(&transformations, p)->inputs[i]=or_dummy;
                        inputs_changed=true;
                    }
                }
            }
            if(inputs_changed){
                // discard all outputs from previous iteration
                for(int i=0; i<vector_index_transformation(&transformations, p)->outputs_count; i++){
                    gc_object_dereference(E, (gc_Object*)vector_index_transformation(&transformations, p)->outputs[i]);
                }
                predict_instruction_output(E, program, &program->code[p], 
                    (char*)program->constants, &dummy_objects_counter, vector_index_transformation(&transformations, p));
                for(int i=0; i<vector_index_transformation(&transformations, p)->outputs_count; i++){
                    vector_push(&stack, &vector_index_transformation(&transformations, p)->outputs[i]);
                    // output is referenced by the transformation
                    gc_object_reference((gc_Object*)vector_index_transformation(&transformations, p)->outputs[i]);
                }
            } else {
                for(int i=0; i<vector_index_transformation(&transformations, p)->outputs_count; i++){
                    Dummy* output=vector_index_transformation(&transformations, p)->outputs[i];
                    vector_push(&stack, &output);
                }
            }
        }
    }
    if(print_optimisations){
        printf("Deduced flow chart:\n");
        print_transformations(vector_get_data(&instructions), vector_get_data(&transformations));
        printf("\n");
    }

    // step 2: perform optimisations

    BytecodeManipulation manipulation;
    manipulation.instructions=&instructions;
    manipulation.transformations=&transformations;
    manipulation.informations=&informations;
    manipulation.constants=&constants;
    manipulation.executor=E;
    manipulation.dummy_objects_counter=&dummy_objects_counter;
    manipulation.program=program;
    manipulation.print_optimisations=print_optimisations;
    
    if(E->options.optimise_stack_operations) {
        stack_usage_optimisations(E, program, &manipulation, &instructions, &transformations, &constants, print_optimisations);
    }
    if(E->options.optimise_jump_to_return){
        jump_to_return(&manipulation, &instructions, &transformations);
    }
    for(int pointer=count_instructions((Instruction*)vector_get_data(&instructions))-1; pointer>=1; pointer--){
        // here short circuit evaluation is used so that if one of optimisations succeeds the rest is not performed on the same instruction
        allow_unused_variable((void*)(// prevent emitting "value computed is not used" warning
           (E->options.optimise_tail_calls
                && tail_calls(&manipulation, &instructions, pointer, print_optimisations))
        || (E->options.remove_useless_operations 
                && remove_useless_operations(&manipulation, &instructions, &transformations, pointer, print_optimisations))
        || (E->options.constants_folding 
                && constants_folding(E, &manipulation, &instructions, &transformations, &constants, &pointer, print_optimisations))
        || (E->options.typed_variants 
                && typed_variants(&instructions, &transformations, pointer))
        ));
    }

    if(print_optimisations){
        printf("Disconnected flow chart:\n");
        print_transformations(vector_get_data(&instructions), vector_get_data(&transformations));
        printf("\n");
    }

    // step 3: add swap instructions to ensure that objects
    // are in right order according to the flow chart made in the first step

    #define STACK(nth) ((Dummy**)vector_index(&stack, vector_count(&stack)-1-(nth)))
    for(int p=bytecode_iterator_start(&progress_state, vector_get_data(&instructions), 0); p!=-1; p=bytecode_iterator_next(&progress_state, vector_get_data(&instructions))){
        if(p==0){
            // remove elements from dummy stack and push items from provided vector to it
            while(!vector_empty(&stack)){
                vector_pop(&stack);
            }
            for(int i=vector_count(&provided)-1; i>=0; i--){
                vector_push(&stack, vector_index(&provided, i));
            }
        }
        for(int i=0; i<gets_from_stack(*vector_index_instruction(&instructions, p)); i++){
            Dummy* top=*STACK(i);
            Dummy* expected=vector_index_transformation(&transformations, p)->inputs[i];
            if(!dummies_compatible(top, expected)){
                int vector_depth=vector_count(&stack);
                for(int j=1; j<vector_depth; j++){
                    if(dummies_compatible(*STACK(j), expected)){
                        Instruction swap_instruction={b_swap};
                        swap_instruction.swap_argument.left=j;
                        swap_instruction.swap_argument.right=i;
                        Transformation swap_transformation;
                        transformation_from_instruction(&swap_transformation, &swap_instruction);
                        Dummy* any=new_any_dummy(E);
                        for(int k=0; k<swap_transformation.inputs_count; k++){
                            if(k==swap_instruction.swap_argument.left || k==swap_instruction.swap_argument.right){
                                swap_transformation.inputs[k]=*STACK(k);
                                gc_object_reference((gc_Object*)swap_transformation.inputs[k]);
                            } else {
                                swap_transformation.inputs[k]=any;
                                gc_object_reference((gc_Object*)swap_transformation.inputs[k]);
                            }
                        }
                        predict_instruction_output(E, program, &swap_instruction, program->constants, &dummy_objects_counter, &swap_transformation);
                        for(int k=0; k<swap_transformation.outputs_count; k++){
                            gc_object_reference((gc_Object*)swap_transformation.outputs[k]);
                        }
                        Dummy* temp=*STACK(swap_instruction.swap_argument.left);
                        *STACK(swap_instruction.swap_argument.left)=*STACK(swap_instruction.swap_argument.right);
                        *STACK(swap_instruction.swap_argument.right)=temp;
                        vector_insert(&instructions, p, &swap_instruction);
                        vector_insert(&transformations, p, &swap_transformation);
                        vector_insert(&informations, p, vector_index(&informations, p));
                        p=bytecode_iterator_next(&progress_state, vector_get_data(&instructions));
                        break;
                    }
                }
            }
        }
        for(int j=0; j<gets_from_stack(*vector_index_instruction(&instructions, p)); j++){
            vector_pop(&stack);
        }
        for(int i=0; i<pushes_to_stack(*vector_index_instruction(&instructions, p)); i++){
            vector_push(&stack, &vector_index_transformation(&transformations, p)->outputs[i]);
        }
    }
    // discard objects that aren't used anywhere
    // this shouldn't happen in a correctly assembled program
    // but it's a way to get extra safety
    /*while(!vector_empty(&stack)){
        unsigned insert_position=vector_count(&instructions)-1;
        Instruction instruction={b_discard};
        vector_insert(&instructions, insert_position, &instruction);
        Transformation transformation;
        transformation_init(&transformation, 1, 0);
        transformation.inputs[0]=*(Dummy**)vector_pop(&stack);
        gc_object_reference((gc_Object*)transformation.inputs[0]);
        vector_insert(&transformations, insert_position, &transformation);
        // take information from end instruction
        vector_push(&informations, vector_top(&informations));
    } */
    
    remove_no_ops(E, &instructions, &informations, &transformations);

    if(print_optimisations){
        printf("\nFinal flow chart:\n");
        print_transformations(vector_get_data(&instructions), vector_get_data(&transformations));
    }

    // cleanup and moving data back from vectors to BytecodeProgram

    while(!vector_empty(&provided)){
        gc_object_dereference(E, (gc_Object*)*(Dummy**)vector_pop(&provided));
    }
    vector_deinit(&provided);
    vector_deinit(&stack);// dummy stack doesn't own any of it's objects
    for(int i=0; i<vector_count(&transformations); i++){
        transformation_deinit(E, vector_index_transformation(&transformations, i));
    }
    vector_deinit(&transformations);

    program->code=(Instruction*)vector_get_data(&instructions);
    program->information=(InstructionInformation*)vector_get_data(&informations);
    program->constants_size=vector_count(&constants);
    program->constants=(char*)vector_get_data(&constants);

    #undef FILL_WITH_NO_OP
}

#undef LOG_IF_ENABLED