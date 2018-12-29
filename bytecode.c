#include "bytecode.h"

char* INSTRUCTION_NAMES[]={
    "end",
    "discard",
    "swap",
    "double",
    "load_string",
    "load_number",
    "table_literal",
    "null",
    "function",
    "return",
    "get_scope",
    "set_scope",
    "label",
    "jump",
    "jump_not",
    "get",
    "set",
    "call",
    "unary",
    "prefix"
};

void stringify_instruction(const bytecode_program* prog, char* destination, instruction instr, int index, int buffer_count){
    switch(instr.type){
        case b_end:
        case b_discard:
        case b_get_scope:
        case b_set_scope:
        case b_null:
        case b_return:
        case b_unary:
        case b_prefix:
            snprintf(destination, buffer_count, "%i: %s\n", index, INSTRUCTION_NAMES[instr.type]);// these instructions doesn't use the argument
            break;
        case b_load_number:
            snprintf(destination, buffer_count, "%i: %s %f\n", index, INSTRUCTION_NAMES[instr.type], *((float*)&instr.argument));
            break;
        case b_load_string:
            snprintf(destination, buffer_count, "%i: %s %li \"%s\"\n", index, INSTRUCTION_NAMES[instr.type], instr.argument, ((char*)prog->constants)+instr.argument);// displays string value
            break;
        default:
            snprintf(destination, buffer_count, "%i: %s %li\n", index, INSTRUCTION_NAMES[instr.type], instr.argument);
    }
}

char* stringify_bytecode(const bytecode_program* prog){
    int pointer=0;
    int string_end=0;
    int result_size=64;
    char* result=calloc(64, sizeof(char));
    CHECK_ALLOCATION(result);

    while(prog->code[pointer].type!=b_end){
        char stringified_instruction[64];
        stringify_instruction(prog, (char*)&stringified_instruction, prog->code[pointer], pointer, 64);
        int stringified_length=strlen(stringified_instruction);

        if(result_size-string_end<=stringified_length){
            result_size*=2;
            result=realloc(result, result_size);
        }
        strncat(result, stringified_instruction, result_size);
        string_end+=strlen(stringified_instruction);
        pointer++;
    }
    result[string_end]='\0';
    return result; 
}

#define X(t, result) case t: return result;
int gets_from_stack(instruction instr){
    switch(instr.type){
        X(b_discard, 1)
        X(b_swap, 1)
        X(b_function, 1 )
        X(b_return, 1)
        X(b_set_scope, 1)
        X(b_get, instr.argument+1)
        X(b_set, instr.argument+2)
        X(b_call, instr.argument+1)
        X(b_unary, 3)
        X(b_prefix, 2)
        default: return 0;
    }
}
#undef X

#define X(i) || instr==i
bool pushes_to_stack(instruction_type instr){
    return !(false
    X(b_end)
    X(b_discard)
    X(b_get_scope)
    X(b_label)
    X(b_jump)
    X(b_jump_not)
    );
}
bool changes_flow(instruction_type instr){
    return false
    X(b_return)
    X(b_label)
    X(b_jump)
    X(b_jump_not);
}
bool changes_scope(instruction_type instr){
    return false
    X(b_return)
    X(b_get_scope)
    X(b_set_scope);
}
#undef X