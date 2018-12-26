#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "macros.h"

typedef enum instruction_type instruction_type;
enum instruction_type {
    b_end,// denotes the end of the program
    b_discard,// stack: [value_to_discard] - removes one value from the stack
    b_swap,// performs argument number of swaps starting from stack.top-argument
    b_double,// creates a copy of topmost stack item
    b_load_string,// argument: position_in_constants
    b_load_number,// argument: bits of float value
    b_table_literal,
    b_null,
    b_function,// argument: beginning_label, stack: [arguments_count]
    b_return,
    b_get_scope,
    b_set_scope,
    b_label,
    b_jump,// argument: position
    b_jump_not,// argument: position
    b_get,// stack: [key, ?indexed_object] - if argument is zero use scope as indexed object
    b_set,// stack: [value, key, ?indexed_object] - if argument is zero use scope as indexed object, pushes the value back to the stack
    b_call,// argument: number_of_arguments, stack: [function, arguments...]
    b_unary,// stack: [a, b, operator]
    b_prefix,// stack: [a, operator]
};

typedef struct instruction instruction;
struct instruction {
    instruction_type type;
    long argument;// long type ensures that 4bit float will fit
};

typedef struct {
    instruction* code;
    void* constants;
} bytecode_program;

char* stringify_bytecode(const bytecode_program* program);

#endif