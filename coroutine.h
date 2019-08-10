#ifndef COROUTINE_H
#define COROUTINE_H

#include "execution.h"

Object new_coroutine(Executor* E, Object function, Object* arguments, int arguments_count);

#endif