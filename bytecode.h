#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "utility.h"
#include "datatypes/stream.h"

// [] - arguments on stack
#define INSTRUCTION_TYPES \
    X(end) /*           denotes the end of the program */ \
    X(no_op) /*         used in optimisation proccess, does nothing */ \
    X(discard) /*       removes one value from the stack */ \
    X(move_top) /*      moves the top Object on the top down the stack by argument move_top(ABC, 1)=ACB move_top(ABC, 2)=CAB */ \
    X(push_to_top)/*    moves the Object lying argument number of items from the top to the top move_top(ABC, 1)=ACB move_top(ABC, 2)=BCA */ \
    X(double) /*        creates a copy of topmost stack item */ \
    X(load_string) /*   argument: position_in_constants */ \
    X(load_number) /*   argument: bits of float value */ \
    X(table_literal) \
    X(null) \
    X(function) /*      argument: sub_program_index, [arguments_count, is_variadic] */ \
    X(return) /*        [object_to_return] */ \
    X(get_scope) \
    X(set_scope) /*     sets the Object on the stack as the current scope */ \
    X(new_scope) /*     creates a new Table and sets it as the current scope */ \
    X(label) /*         no effect, argument: label_index */ \
    X(jump) /*          jump to a label argument: label_index */ \
    X(jump_not) /*      jump to a label if value on stack is falsy argument: label_index */ \
    X(get) /*           get the value at the key from the current scope, [key] */ \
    X(table_get) /*     get the value at the key from the Table, [key, Table] */ \
    X(set) /*           sets field at key in the current scope to value, argument: is_used_in_closure, [key, value] */ \
    X(table_set) /*     sets field at key in table to value, keeps the value on stack, [key, Table, value] */ \
    X(table_set_keep) /*same as table_set but keeps the indexed Table on the stack, [key, Table, value] */ \
    X(call) /*          argument: number_of_arguments, [function, arguments...] */ \
    X(binary) /*         [a, b, operator] */ \
    X(prefix) /*        [a, operator] */

typedef enum {
    #define X(t) b_##t,
    INSTRUCTION_TYPES
    #undef X
} InstructionType;

typedef struct {
    unsigned line;
    unsigned column;
    unsigned file;// position in constants containing file name string
    unsigned comment;
} InstructionInformation;

typedef struct {
    InstructionType type;
    long argument;// long type ensures that 4bit float will fit
} Instruction;

typedef struct BytecodeProgram BytecodeProgram;
struct BytecodeProgram {
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
void bytecode_program_free(BytecodeProgram* prog);
void bytecode_program_copy(const BytecodeProgram* source, BytecodeProgram* copy);

int gets_from_stack(Instruction instr);
bool pushes_to_stack(InstructionType instr);
bool changes_flow(InstructionType instr);
bool changes_scope(InstructionType instr);

#endif