#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "macros.h"
#include "datatypes/stream.h"

typedef enum instruction_type instruction_type;

// [] - arguments on stack
#define INSTRUCTION_TYPES \
    X(end) /*           denotes the end of the program */ \
    X(no_op) /*         used in optimisation proccess, does nothing */ \
    X(discard) /*       removes one value from the stack */ \
    X(move_top) /*      moves the top object on the top down the stack by argument move_top(ABC, 1)=ACB move_top(ABC, 2)=CAB */ \
    X(push_to_top)/*    moves the object lying argument number of items from the top to the top move_top(ABC, 1)=ACB move_top(ABC, 2)=BCA */ \
    X(double) /*        creates a copy of topmost stack item */ \
    X(load_string) /*   argument: position_in_constants */ \
    X(load_number) /*   argument: bits of float value */ \
    X(table_literal) \
    X(null) \
    X(function) /*      argument: sub_program_index, [arguments_count, is_variadic] */ \
    X(return) /*        [object_to_return] */ \
    X(get_scope) \
    X(set_scope) /*     sets the object on the stack as the current scope */ \
    X(new_scope) /*     creates a new table and sets it as the current scope */ \
    X(label) /*         no effect, argument: label_index */ \
    X(jump) /*          jump to a label argument: label_index */ \
    X(jump_not) /*      jump to a label if value on stack is falsy argument: label_index */ \
    X(get) /*           get the value at the key from the current scope, [key] */ \
    X(table_get) /*     get the value at the key from the table, [key, table] */ \
    X(set) /*           sets field at key in the current scope to value, argument: is_used_in_closure, [key, value] */ \
    X(table_set) /*     sets field at key in table to value, keeps the value on stack, [key, table, value] */ \
    X(table_set_keep) /*same as table_set but keeps the indexed table on the stack, [key, table, value] */ \
    X(call) /*          argument: number_of_arguments, [function, arguments...] */ \
    X(unary) /*         [a, b, operator] */ \
    X(prefix) /*        [a, operator] */

enum instruction_type {
    #define X(t) b_##t,
    INSTRUCTION_TYPES
    #undef X
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
    int* labels;
    instruction_information* information;
    char* constants;
    bytecode_program* sub_programs;
    int sub_programs_count;
};

void stringify_instruction(const bytecode_program* prog, char* destination, instruction instr, int buffer_count);
char* stringify_bytecode(const bytecode_program* prog);
void bytecode_program_deinit(bytecode_program* prog);

int gets_from_stack(instruction instr);
bool pushes_to_stack(instruction_type instr);
bool changes_flow(instruction_type instr);
bool changes_scope(instruction_type instr);

#endif