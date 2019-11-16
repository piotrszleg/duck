#ifndef AST_TO_BYTECODE_H
#define AST_TO_BYTECODE_H

#include "../parser/ast.h"
#include "../parser/parser.h"
#include "../bytecode/bytecode_program.h"
#include "../error/error.h"
#include "../containers/stream.h"
#include <stdbool.h>

BytecodeProgram* ast_function_to_bytecode(FunctionDeclaration* d);
BytecodeProgram* ast_to_bytecode(Expression* expression, bool keep_scope);

typedef struct {
    stream code;
    unsigned* labels;
    stream information;
    InstructionInformation last_information;
    stream constants;
    stream sub_programs;
    stream upvalues;
} BytecodeTranslation;

#endif