#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
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

typedef struct instruction_information instruction_information;
struct instruction_information {
    unsigned line;
    unsigned column;
    unsigned file;// position in constants containing file name string
    unsigned comment;
};

typedef struct instruction instruction;
struct instruction {
    instruction_type type;
    long argument;// long type ensures that 4bit float will fit
};

typedef struct bytecode_program bytecode_program;
struct bytecode_program {
    instruction* code;
    instruction_information* information;
    char* constants;
};

char* stringify_bytecode(const bytecode_program* program);

int gets_from_stack(instruction instr);
bool pushes_to_stack(instruction_type instr);
bool changes_flow(instruction_type instr);
bool changes_scope(instruction_type instr);

#endif