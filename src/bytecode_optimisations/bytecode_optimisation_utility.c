#include "bytecode_optimisation_utility.h"

unsigned count_instructions(Instruction* code){
    unsigned p=0;
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
            INCORRECT_ENUM_VALUE(Assumption.Type, assumption, assumption->type);
            return NULL;
    }
}

bool instruction_is_constant(Instruction* instruction, Transformation* transformation) {
    switch(instruction->type) {
        case b_null:
        case b_load_int:
        case b_load_float:
        case b_load_string:
        case b_table_literal:
        case b_function_1:
        case b_function_2:
        case b_double:
        case b_get:
        case b_discard:
            return true;
        #define BINARY(instruction, op) \
            case instruction: \
                return operator_predict_result(dummy_type(transformation->inputs[0]), dummy_type(transformation->inputs[1]), op)!=tu_unknown;
        #define PREFIX(instruction, op) \
            case instruction: \
                return operator_predict_result(t_null, dummy_type(transformation->inputs[0]), op)!=tu_unknown;
        OPERATOR_INSTRUCTIONS
        #undef BINARY
        #undef PREFIX
        case b_binary:
            return transformation->inputs[0]->type==d_constant 
            && transformation->inputs[0]->constant_value.type==t_string
            && operator_predict_result(dummy_type(transformation->inputs[1]), dummy_type(transformation->inputs[2]), transformation->inputs[0]->constant_value.text)!=tu_unknown;
        case b_prefix:
            return transformation->inputs[0]->type==d_constant 
            && transformation->inputs[0]->constant_value.type==t_string
            && operator_predict_result(tu_null, dummy_type(transformation->inputs[1]), transformation->inputs[0]->constant_value.text)!=tu_unknown;
        default: return false;
    }
}

void print_transformations(Instruction* instructions, Transformation* transformations){
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
    heap_object_reference((HeapObject*)transformation.outputs[0]);
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

#undef VECTOR_INDEX_FUNCTION