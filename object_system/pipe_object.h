#ifndef PIPE_OBJECT_H
#define PIPE_OBJECT_H

#include "object.h"
#include "object_operations.h"
#include "object_utility.h"

object new_pipe(executor* Ex, object f1, object f2);

#endif