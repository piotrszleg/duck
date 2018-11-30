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
    b_swap,// stack: [a, b] - swaps two values at the top of the stack
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
    b_call,// stack: [function, arguments...]
    b_unary,// stack: [a, b, operator]
    b_prefix,// stack: [a, operator]
};

typedef struct instruction instruction;
struct instruction {
    instruction_type type;
    long argument;// long type ensures that 4bit float will fit
};

typedef struct bytecode_program bytecode_program;
struct bytecode_program {
    instruction* code;
    void* constants;
};

char* stringify_bytecode(bytecode_program);

#endif