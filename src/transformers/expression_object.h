#ifndef EXPRESSION_OBJECT_H
#define EXPRESSION_OBJECT_H

#include "../parser/ast/ast_visitor.h"
#include "../object_system/object.h"

Expression* object_to_literal(Executor* E, Object o);
Object expression_to_object(Executor* E, Expression* expression);
Expression* object_to_expression(Executor* E, Object o);

#endif