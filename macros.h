#ifndef MACROS_H
#define MACROS_H

#include "parser/ast_visitor.h"
#include "datatypes/map.h"
#include "execution.h"
#include "runtime/struct_descriptor.h"

void execute_macros(Executor* E, expression* ast);

#endif