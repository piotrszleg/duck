#ifndef OBJECT_UTILITY_H
#define OBJECT_UTILITY_H

#include "object.h"
#include "object_operations.h"

void set_function(executor* Ex, object t, const char* name, int arguments_count, bool variadic, object_system_function f);

#endif