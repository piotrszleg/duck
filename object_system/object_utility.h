#ifndef OBJECT_UTILITY_H
#define OBJECT_UTILITY_H

#include "object.h"
#include "object_operations.h"

void set_function(Executor* E, Object t, const char* name, int arguments_count, bool variadic, ObjectSystemFunction f);

#endif