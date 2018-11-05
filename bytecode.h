#ifndef BYTECODE_H
#define BYTECODE_H

#include "parser/ast.h"
#include "object_system/object.h"
#include "object_system/error.h"

typedef struct stream stream;
struct stream {
    void* data;
    size_t position;// position inside of data in bytes
    size_t size;// size of the stream in bytes
};

#define STACK_SIZE 10

typedef struct vm_stack vm_stack;
struct vm_stack {
    object* items[STACK_SIZE];
    int pointer;
};

typedef enum instruction_type instruction_type;
enum instruction_type {
    b_end,
    b_discard,
    b_swap,
    b_load_string,// position_in_constants
    b_load_number,// position_in_constants
    b_table_literal,
    b_function_literal,// beginning_label
    b_get_scope,
    b_set_scope,
    b_label,
    b_jump,// position
    b_jump_not,// position
    b_get,
    b_set,
    b_call,// (arguments and function on the stack)
    b_operator,// is_prefix
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

bytecode_program ast_to_bytecode(expression* exp, int keep_scope);
object* execute_bytecode(instruction* code, void* constants, table* scope);
char* stringify_bytecode(instruction* code);

#endif