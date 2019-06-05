#ifndef BINDING_OBJECT_H
#define BINDING_OBJECT_H

#include "object.h"
#include "object_operations.h"
#include "object_utility.h"

object new_binding(executor* Ex, object f, object arguments);

#endif