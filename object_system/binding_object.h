#ifndef BINDING_OBJECT_H
#define BINDING_OBJECT_H

#include "object.h"
#include "object_operations.h"
#include "object_utility.h"

Object to_binding(Executor* E, Object f, Object argument);
Object new_binding(Executor* E, Object scope, Object* arguments, int arguments_count);

#endif