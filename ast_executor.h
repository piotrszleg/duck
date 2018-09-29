#ifndef AST_EXECUTOR_H
#define AST_EXECUTOR_H

#include "parser/ast.h"
#include "object_system/object.h"
#include "object_system/error.h"

object* execute_ast(expression* exp, table* scope);

#endif