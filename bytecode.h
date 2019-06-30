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
    X(move_top) /*      (uint_argument) moves the object on the top of the stack by argument move_top(ABC, 1)=ACB move_top(ABC, 2)=CAB */ \
    X(push_to_top)/*    (uint_argument) moves the Object lying argument number of items from the top to the top move_top(ABC, 1)=ACB move_top(ABC, 2)=BCA */ \
    X(double) /*        creates a copy of topmost stack item */ \
    X(load_string) /*   (uint_argument position_in_constants) */ \
    X(load_int) /*      (int_argument) */ \
    X(load_float) /*    (float_argument) */ \
    X(table_literal) \
    X(null) \
    X(pre_function)/*   (pre_function_argument) */ \
    X(function) /*      (uint_argument: sub_program_index */ \
    X(return) /*        [object_to_return] */ \
    X(get_scope) /*     pushes current scope onto the stack */ \
    X(set_scope) /*     [scope] sets the Object on the stack as the current scope */ \
    X(new_scope) /*     creates a new Table and sets it as the current scope */ \
    X(label) /*         (uint_argument label_index) sets label */ \
    X(jump) /*          (uint_argument label_index) jumps to label */ \
    X(jump_not) /*      [value] (uint_argument label_index) jump to a label if value on stack is falsy */ \
    X(get) /*           get the value at the key from the current scope, [key] */ \
    X(table_get) /*     get the value at the key from the Table, [key, Table] */ \
    X(set) /*           (bool_argument is_used_in_closure) [key, value] sets field at key in the current scope to value  */ \
    X(table_set) /*     [key, Table, value] sets field at key in table to value, keeps the value on stack */ \
    X(table_set_keep) /*[key, Table, value] same as table_set but keeps the indexed Table on the stack */ \
    X(call) /*          (uint_argument number_of_arguments) [function, arguments...] */ \
    X(tail_call) \
    X(binary) /*        [a, b, operator] */ \
    X(message) /*       (arguments_count) [messaged, message_identifier, arguments...] */\
    X(prefix) /*        [a, operator] */ \
    X(swap) /*          (swap_argument) */ \
    X(question_mark) /* [expression] */

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
} PreFunctionArgument;

typedef struct{
    unsigned char left;
    unsigned char right;
} SwapArgument;

typedef struct {
    InstructionType type;
    union {
        PreFunctionArgument pre_function_argument;
        SwapArgument swap_argument;
        float float_argument;
        unsigned int uint_argument;
        int int_argument;
        bool bool_argument;
    };
} Instruction;

typedef struct BytecodeProgram BytecodeProgram;
struct BytecodeProgram {
    gc_Pointer gcp;
    char* source_file_name;
    Instruction* code;
    int* labels;
    InstructionInformation* information;
    char* constants;
    int constants_size;
    BytecodeProgram* sub_programs;
    int sub_programs_count;
};

void stringify_instruction(const BytecodeProgram* prog, char* destination, Instruction instr, int buffer_count);
char* stringify_bytecode(const BytecodeProgram* prog);
void bytecode_program_init(Executor* E, BytecodeProgram* program);
void bytecode_program_free(BytecodeProgram* prog);
void list_program_labels(BytecodeProgram* program);
void bytecode_program_copy(const BytecodeProgram* source, BytecodeProgram* copy);

int gets_from_stack(Instruction instr);
int pushes_to_stack(Instruction instr);
bool changes_flow(InstructionType instr);
bool changes_scope(InstructionType instr);
bool finishes_program(InstructionType instr);
bool carries_stack(InstructionType instr);

#endif