#ifndef PIPE_OBJECT_H
#define PIPE_OBJECT_H

#include "object.h"
#include "object_operations.h"
#include "object_utility.h"

Object new_pipe(Executor* E, Object f1, Object f2);

#endif