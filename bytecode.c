#include "bytecode.h"

char* INSTRUCTION_NAMES[]={
    "end",
    "discard",
    "swap",
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

void stringify_instruction(bytecode_program prog, char* destination, instruction instr, int index, int buffer_count){
    switch(instr.type){
        case b_end:
        case b_discard:
        case b_swap:
        case b_get_scope:
        case b_set_scope:
        case b_call:
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
            snprintf(destination, buffer_count, "%i: %s %li \"%s\"\n", index, INSTRUCTION_NAMES[instr.type], instr.argument, ((char*)prog.constants)+instr.argument);// displays string value
            break;
        default:
            snprintf(destination, buffer_count, "%i: %s %li\n", index, INSTRUCTION_NAMES[instr.type], instr.argument);
    }
}

char* stringify_bytecode(bytecode_program prog){
    int pointer=0;
    int string_end=0;
    int result_size=64;
    char* result=calloc(64, sizeof(char));
    CHECK_ALLOCATION(result);

    while(prog.code[pointer].type!=b_end){
        char stringified_instruction[64];
        stringify_instruction(prog, (char*)&stringified_instruction, prog.code[pointer], pointer, 64);
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