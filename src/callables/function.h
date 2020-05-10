#ifndef FUNCTION_H
#define FUNCTION_H

#include "../execution/execution.h"

bool is_arguments_count_correct(Executor* E, Function* function, int arguments_count, Object* error);
Object call_function(Executor* E, Function* f, Object* arguments, int arguments_count);

#endif