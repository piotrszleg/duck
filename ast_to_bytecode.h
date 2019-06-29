#ifndef AST_TO_BYTECODE_H
#define AST_TO_BYTECODE_H

#include "parser/ast.h"
#include "parser/parser.h"
#include "bytecode.h"
#include "error/error.h"
#include "datatypes/stream.h"
#include <stdbool.h>

BytecodeProgram ast_to_bytecode(expression* exp, bool keep_scope);

typedef struct {
    stream code;
    unsigned* labels;
    stream information;
    InstructionInformation last_information;
    stream constants;
    stream sub_programs;
} BytecodeTranslation;

#endif