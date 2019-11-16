#ifndef FUNCTION_H
#define FUNCTION_H

#include "../execution/execution.h"

Object call_function(Executor* E, Function* f, Object* arguments, int arguments_count);

#endif