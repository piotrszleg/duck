#include "bytecode_program.h"

char* INSTRUCTION_NAMES[]={
    #define X(t) #t,
    INSTRUCTION_TYPES
    #undef X
};

#define X(t, result) case t: return result;
int gets_from_stack(Instruction instr){
    switch(instr.type){
        X(b_discard, 1)
        X(b_double, 1)
        X(b_push_to_top, instr.uint_argument+1)
        X(b_move_top, instr.uint_argument+1)
        X(b_return, 1)
        X(b_end, 1)
        X(b_set_scope, 1)
        X(b_get, 1)
        X(b_table_get, 2)
        X(b_set, 2)
        X(b_table_set, 3)
        X(b_table_set_keep, 3)
        X(b_call, instr.uint_argument+1)
        X(b_tail_call, instr.uint_argument+1)
        X(b_message, instr.uint_argument+2)
        X(b_binary, 3)
        X(b_prefix, 2)
        X(b_function, 1)
        X(b_jump_not, 1)
        X(b_swap, MAX(instr.swap_argument.left, instr.swap_argument.right)+1)
        default: return 0;
    }
}

int pushes_to_stack(Instruction instr){
    switch(instr.type){
        X(b_double, 2)
        X(b_push_to_top, instr.uint_argument+1)
        X(b_move_top, instr.uint_argument+1)
        X(b_swap, MAX(instr.swap_argument.left, instr.swap_argument.right)+1)
        X(b_end, 0)
        X(b_no_op, 0)
        X(b_discard, 0)
        X(b_set_scope, 0)
        X(b_label, 0)
        X(b_jump, 0)
        X(b_jump_not, 0)
        X(b_new_scope, 0)
        X(b_return, 0)
        X(b_tail_call, 0)
        default: return 1;
    }
}
#undef X

#define X(i) || instr==i
bool changes_flow(InstructionType instr){
    return false
    X(b_jump)
    X(b_jump_not);
}
bool changes_scope(InstructionType instr){
    return false
    X(b_new_scope)
    X(b_set_scope);
}
bool finishes_program(InstructionType instr){
    return false
    X(b_end)
    X(b_return)
    X(b_tail_call);
}
bool carries_stack(InstructionType instr){
    return false
    X(b_jump_not)
    X(b_jump)
    X(b_label);
}
bool instruction_is_literal(InstructionType instr){
    return false
    X(b_load_float)
    X(b_load_int)
    X(b_load_string)
    X(b_null)
    X(b_function);
}
#undef X