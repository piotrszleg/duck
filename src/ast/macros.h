#ifndef MACROS_H
#define MACROS_H

#include "../parser/ast_visitor.h"
#include "../containers/map.h"
#include "../execution/execution.h"
#include "../runtime/struct_descriptor.h"

// returns error or null if it hasn't occurred
Object execute_macros(Executor* E, Expression** ast);

#endif