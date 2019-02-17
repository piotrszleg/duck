#ifndef AST_TO_BYTECODE_H
#define AST_TO_BYTECODE_H

#include "parser/ast.h"
#include "bytecode.h"
#include "error/error.h"
#include "datatypes/stream.h"
#include "datatypes/stack.h"
#include <stdbool.h>

bytecode_program ast_to_bytecode(expression* exp, int keep_scope);
void optimise_bytecode(bytecode_program* prog);

typedef struct bytecode_translation bytecode_translation;
struct bytecode_translation {
    stream code;
    unsigned* labels;
    stream information;
    instruction_information last_information;
    stream constants;
    stream sub_programs;
} ;

#endif