#ifndef COPYING
#define COPYING

#include "../object.h"
#include "copying_state.h"

Object copy_recursive(Executor* E, Object o, CopyingState* state);
Object copy(Executor* E, Object o);

#endif