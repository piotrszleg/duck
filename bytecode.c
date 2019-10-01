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
        X(b_return, 1)
        X(b_end, 1)
        X(b_leave_scope, 1)
        X(b_get, 1)
        X(b_table_get, 2)
        X(b_set, 2)
        X(b_table_set, 3)
        X(b_table_set_keep, 3)
        X(b_call, instr.uint_argument+1)
        X(b_tail_call, instr.uint_argument+1)
        X(b_native_call_1, instr.uint_argument)
        X(b_native_tail_call_1, instr.uint_argument)
        X(b_function_2, 1)
        X(b_jump_not, 1)
        X(b_swap, MAX(instr.swap_argument.left, instr.swap_argument.right)+1)
        X(b_binary, 3)
        X(b_add, 2)
        X(b_subtract, 2)
        X(b_multiply, 2)
        X(b_divide, 2)
        X(b_divide_floor, 2)
        X(b_modulo, 2)
        X(b_add_int, 2)
        X(b_subtract_int, 2)
        X(b_multiply_int, 2)
        X(b_divide_int, 2)
        X(b_divide_floor_int, 2)
        X(b_modulo_int, 2)
        X(b_add_float, 2)
        X(b_subtract_float, 2)
        X(b_multiply_float, 2)
        X(b_divide_float, 2)
        X(b_minus_float, 2)
        X(b_add_string, 2)
        X(b_prefix, 2)
        X(b_minus, 1)
        X(b_minus_int, 1)
        X(b_not, 1)
        default: return 0;
    }
}

int pushes_to_stack(Instruction instr){
    switch(instr.type){
        X(b_double, 2)
        X(b_push_to_top, instr.uint_argument+1)
        X(b_swap, MAX(instr.swap_argument.left, instr.swap_argument.right)+1)
        X(b_end, 0)
        X(b_no_op, 0)
        X(b_discard, 0)
        X(b_leave_scope, 0)
        X(b_label, 0)
        X(b_jump, 0)
        X(b_jump_not, 0)
        X(b_new_scope, 0)
        X(b_return, 0)
        X(b_tail_call, 0)
        X(b_native_call_1, 0)
        X(b_native_tail_call_1, 0)
        X(b_native_tail_call_2, 0)
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
    X(b_function_2);
}
#undef X

int find_label(Instruction* code, int index){
    for(int search_pointer=0; code[search_pointer].type!=b_end; search_pointer++){
        if(code[search_pointer].type==b_label && code[search_pointer].uint_argument==index){
            return search_pointer;
        }
    }
    return -1;
}

int bytecode_iterator_start(BytecodeIterator* iterator, Instruction* code, unsigned start){
    iterator->start=start;
    iterator->last=start;
    vector_init(&iterator->branches, sizeof(int), 8);
    iterator->revisit=false;
    return 0;
}

int bytecode_iterator_next(BytecodeIterator* iterator, Instruction* code){
    vector* branches=&iterator->branches;
    unsigned index=iterator->last;
    if(finishes_program(code[index].type)){
        if(iterator->revisit){
            iterator->revisit=false;
            index=iterator->start;
            iterator->last=index;
            return index;
        } else {
            vector_deinit(branches);
            return -1;
        }
    }
    if(code[index].type==b_jump_not){
        if(vector_search(branches, &index)>0){
            index=find_label(code, code[index].uint_argument);
        } else {
            vector_push(branches, &index);
            iterator->revisit=true;
            index++;
        }
    } else if(code[index].type==b_jump){
        index=find_label(code, code[index].uint_argument);
    } else {
        index++;
    }
    iterator->last=index;
    return index;
}