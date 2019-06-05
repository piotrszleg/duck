#ifndef BINDING_OBJECT_H
#define BINDING_OBJECT_H

#include "object.h"
#include "object_operations.h"
#include "object_utility.h"

Object new_binding(Executor* E, Object f, Object arguments);

#endif