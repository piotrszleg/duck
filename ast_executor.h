#ifndef AST_EXECUTOR_H
#define AST_EXECUTOR_H

#include "parser/ast.h"
#include "object_system/object.h"
#include "object_system/error.h"
#include "builtins.h"

object* execute_ast(expression* exp, table* scope, int create_block_subscope);
extern int current_line;

#endif