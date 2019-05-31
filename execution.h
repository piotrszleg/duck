#ifndef EXECUTION_H
#define EXECUTION_H

#include <stdbool.h>
#include <string.h>
#include "bytecode.h"
#include "ast_to_bytecode.h"
#include "execute_bytecode.h"
#include "optimisations/ast_optimisations.h"
#include "optimisations/bytecode_optimisations.h"
#include "runtime/builtins.h"
#include "utility.h"
#include "object_system/object_operations.h"
#include "error/execution_state.h"
#include "parser/parser.h"
#include "execute_ast.h"
#include "execute_ast.h"
#include "options.h"

object detach_function(function* f);
object evaluate_string(const char* s, object scope);
object evaluate_file(const char* file_name, object scope);
void execute_file(const char* file_name);
object call_function(function* f, object* arguments, int arguments_count);

#endif