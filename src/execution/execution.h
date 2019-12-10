#ifndef EXECUTION_H
#define EXECUTION_H

#include <stdbool.h>
#include <string.h>
#include "../bytecode/bytecode_program.h"
#include "../transformers/ast_to_bytecode.h"
#include "../bytecode/execute_bytecode.h"
#include "../ast/ast_optimisations.h"
#include "../bytecode_optimisations/bytecode_optimisations.h"
#include "../utility.h"
#include "../error/execution_state.h"
#include "../parser/parser.h"
#include "options.h"
#include "../ast/macros.h"
#include "executor.h"

Object evaluate(Executor* E, Expression* parsing_result, Object scope, const char* file_name, bool delete_ast);
Object evaluate_string(Executor* E, const char* s, Object scope);
Object evaluate_file(Executor* E, const char* file_name, Object scope);
void execute_file(Executor* E, const char* file_name);

#include "../runtime/builtins.h"

#endif