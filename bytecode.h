#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "utility.h"
#include "datatypes/stream.h"
#include "object_system/object.h"

// () - instruction argument
// [] - arguments on the stack
#define INSTRUCTION_TYPES \
    X(end) /*           denotes the end of the program */ \
    X(no_op) /*         used in optimisation proccess, does nothing */ \
    X(discard) /*       removes one value from the stack */ \
    X(push_to_top)/*    (uint_argument) moves the Object lying argument number of items from the top to the top push_to_top(ABC, 1)=ACB push_to_top(ABC, 2)=BCA */ \
    X(double) /*        creates a copy of topmost stack item */ \
    X(load_string) /*   (uint_argument position_in_constants) */ \
    X(load_int) /*      (int_argument) */ \
    X(load_float) /*    (float_argument) */ \
    X(table_literal) \
    X(null) \
    X(function_1)/*     (function_argument) function needs to be splitted into two instructions to have all needed data */ \
    X(function_2) /*    (uint_argument sub_program_index) */ \
    X(native_call_1)/*  (uint_argument arguments_count) */ \
    X(native_call_2) /* (uint_argument pointer_to_function) */ \
    X(native_tail_call_1)/*  (uint_argument arguments_count) */ \
    X(native_tail_call_2) /* (uint_argument pointer_to_function) */ \
    X(return) /*        [object_to_return] */ \
    X(enter_scope) /*   pushes current scope into the stack, creates a table and sets it as a new scope */ \
    X(leave_scope) /*   [new_scope] discards the previous scope, sets the object on stack as the new scope */ \
    X(new_scope) /*     creates a new Table and sets it as the current scope */ \
    X(label) /*         (uint_argument label_index) sets label */ \
    X(jump) /*          (uint_argument label_index) jumps to label */ \
    X(jump_not) /*      [value] (uint_argument label_index) jump to a label if value on stack is falsy */ \
    X(get) /*           get the value at the key from the current scope, [key] */ \
    X(table_get) /*     get the value at the key from the table, [key, table] */ \
    X(set) /*           (bool_argument is_used_in_closure) [key, value] sets field at key in the current scope to value  */ \
    X(table_set) /*     [key, table, value] sets field at key in table to value, keeps the value on stack */ \
    X(table_set_keep) /*[key, table, value] same as table_set but keeps the indexed table on the stack */ \
    X(call) /*          (uint_argument number_of_arguments) [function, arguments...] */ \
    X(tail_call) \
    X(binary) /*        [a, b, operator] */ \
    X(prefix) /*        [a, operator] */ \
    X(swap) /*          (swap_argument) */ \
    X(add) /*           [a, b] */ \
    X(subtract) \
    X(multiply) \
    X(divide) \
    X(divide_floor) \
    X(modulo) \
    X(not) \
    X(minus) \
    X(add_int) \
    X(subtract_int) \
    X(multiply_int) \
    X(divide_int) \
    X(divide_floor_int) \
    X(modulo_int) \
    X(minus_int) \
    X(add_float) \
    X(subtract_float) \
    X(multiply_float) \
    X(divide_float) \
    X(minus_float) \
    X(add_string)

#define OPERATOR_INSTRUCTIONS \
    BINARY(b_add, "+") \
    BINARY(b_subtract, "-") \
    BINARY(b_multiply, "*") \
    BINARY(b_divide, "/") \
    BINARY(b_divide_floor, "//") \
    BINARY(b_modulo, "%") \
    BINARY(b_add_int, "+") \
    BINARY(b_subtract_int, "-") \
    BINARY(b_multiply_int, "*") \
    BINARY(b_divide_int, "/") \
    BINARY(b_divide_floor_int, "//") \
    BINARY(b_modulo_int, "%") \
    BINARY(b_add_float, "+") \
    BINARY(b_subtract_float, "-") \
    BINARY(b_multiply_float, "*") \
    BINARY(b_divide_float, "/") \
    BINARY(b_add_string, "+") \
    PREFIX(b_minus, "-") \
    PREFIX(b_minus_int, "-") \
    PREFIX(b_minus_float, "-") \
    PREFIX(b_not, "!")

typedef enum {
    #define X(t) b_##t,
    INSTRUCTION_TYPES
    #undef X
} InstructionType;

extern char* INSTRUCTION_NAMES[];

typedef struct {
    unsigned line;
    unsigned column;
    unsigned file;// position in constants containing file name string
    int comment;// if positive position of instruction comment in constants
} InstructionInformation;

typedef struct{
    unsigned char arguments_count;
    bool is_variadic:1;
} FunctionArgument;

typedef struct{
    unsigned char left;
    unsigned char right;
} SwapArgument;

typedef struct {
    InstructionType type;
    union {
        FunctionArgument function_argument;
        SwapArgument swap_argument;
        float float_argument;
        unsigned int uint_argument;
        int int_argument;
        bool bool_argument;
    };
} Instruction;

int gets_from_stack(Instruction instr);
int pushes_to_stack(Instruction instr);
bool changes_flow(InstructionType instr);
bool finishes_program(InstructionType instr);
bool carries_stack(InstructionType instr);
bool instruction_is_literal(InstructionType instr);
int find_label(Instruction* code, int index);

typedef struct {
    vector branches;
    bool revisit;
    unsigned last;
    unsigned start;
}BytecodeIterator;

int bytecode_iterator_start(BytecodeIterator* iterator, Instruction* code, unsigned start);
int bytecode_iterator_next(BytecodeIterator* iterator, Instruction* code);

#define BYTECODE_FOR(iterator_state, index, instructions) \
    for(index=bytecode_iterator_start(&progress_state, (instructions), 0); index!=-1; \
        index=bytecode_iterator_next(&progress_state, (instructions)))

#endif