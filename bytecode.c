#include "bytecode.h"

char* INSTRUCTION_NAMES[]={
    #define X(t) #t,
    INSTRUCTION_TYPES
    #undef X
};

void stringify_instruction(const bytecode_program* prog, char* destination, instruction instr, int buffer_count){
    switch(instr.type){
        case b_end:
        case b_discard:
        case b_get_scope:
        case b_set_scope:
        case b_null:
        case b_return:
        case b_unary:
        case b_prefix:
        case b_new_scope:
        case b_table_set:
        case b_table_set_keep:
        case b_get:
        case b_table_get:
            snprintf(destination, buffer_count, "%s\n", INSTRUCTION_NAMES[instr.type]);// these instructions doesn't use the argument
            break;
        case b_load_number:
            snprintf(destination, buffer_count, "%s %f\n", INSTRUCTION_NAMES[instr.type], *((float*)&instr.argument));
            break;
        case b_load_string:
            snprintf(destination, buffer_count, "%s %li \"%s\"\n", INSTRUCTION_NAMES[instr.type], instr.argument, ((char*)prog->constants)+instr.argument);// displays string value
            break;
        default:
            snprintf(destination, buffer_count, "%s %li\n", INSTRUCTION_NAMES[instr.type], instr.argument);
    }
}

char* stringify_bytecode(const bytecode_program* prog){
    stream s;
    init_stream(&s, 64);

    int pointer=0;
    while(prog->code[pointer].type!=b_end){
        char line_number[8];
        snprintf(line_number, 8, "%i: ", pointer);

        stream_push(&s, line_number, strlen(line_number));

        char stringified_instruction[64];
        stringify_instruction(prog, (char*)&stringified_instruction, prog->code[pointer], 64);
        int stringified_length=strlen(stringified_instruction);

        stream_push(&s, stringified_instruction, stringified_length*sizeof(char));
        pointer++;
    }
    
    if(prog->sub_programs_count>0){

        for(int i=0; i<prog->sub_programs_count; i++){
            char buf[32];// assuming that the number of subprograms has less than 16 digits
            snprintf(buf, 32, "SUBPROGRAM[%i]\n", i);
            char* sub_program_stringified=stringify_bytecode(prog->sub_programs+i);
            stream_push(&s, buf, strlen(buf)*sizeof(char));
            stream_push(&s, sub_program_stringified, strlen(sub_program_stringified)*sizeof(char));
        }
    }
    stream_push(&s, "\0", sizeof(char));
    return s.data;
}

void bytecode_program_deinit(bytecode_program* prog) {
    free(prog->code);
    free(prog->information);
    free(prog->constants);
}

#define X(t, result) case t: return result;
int gets_from_stack(instruction instr){
    switch(instr.type){
        X(b_discard, 1)
        //X(b_move_top, 1)
        X(b_function, 2 )
        X(b_return, 1)
        X(b_set_scope, 1)
        X(b_get, 1)
        X(b_table_get, 2)
        X(b_set, 2)
        X(b_table_set, 3)
        X(b_table_set_keep, 3)
        X(b_call, instr.argument+1)
        X(b_unary, 3)
        X(b_prefix, 2)
        default: return 0;
    }
}
#undef X

#define X(i) || instr==i
bool pushes_to_stack(instruction_type instr){
    // the following instructions DON'T push to the stack
    return !(false
    X(b_move_top)
    X(b_push_to_top)
    X(b_end)
    X(b_no_op)
    X(b_discard)
    X(b_get_scope)
    X(b_set_scope)
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
    X(b_new_scope)
    X(b_get_scope)
    X(b_set_scope);
}
#undef X