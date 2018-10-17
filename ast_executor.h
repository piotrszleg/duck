#ifndef AST_EXECUTOR_H
#define AST_EXECUTOR_H

#include "parser/ast.h"
#include "object_system/object.h"
#include "object_system/error.h"

object* execute_ast(expression* exp, table* scope, int create_block_subscope);
void register_globals(table* scope);
extern int current_line;

#endif