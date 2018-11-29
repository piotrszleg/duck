#ifndef BYTECODE_H
#define BYTECODE_H

#include "parser/ast.h"
#include "object_system/object.h"
#include "error/error.h"
#include "datatypes/stream.h"
#include "datatypes/stack.h"
#include "builtins.h"

typedef enum instruction_type instruction_type;
enum instruction_type {
    b_end,
    b_discard,
    b_swap,
    b_load_string,// position_in_constants
    b_load_number,
    b_table_literal,
    b_null,
    b_function,// beginning_label
    b_return,
    b_get_scope,
    b_set_scope,
    b_label,
    b_jump,// position
    b_jump_not,// position
    b_get,// (key and table on the stack)
    b_set,// (value, key and table on the stack)
    b_call,// (arguments and function on the stack)
    b_unary,// (two arguments and operator on the stack)
    b_prefix,// (one argument and operator on the stack)
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
char* stringify_bytecode(bytecode_program);

#endif