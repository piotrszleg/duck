#ifndef COROUTINE_H
#define COROUTINE_H

#include "../execution/execution.h"

Object new_coroutine(Executor* E, Object function, Object* arguments, int arguments_count);
void coroutine_foreach_children(Executor* E, Coroutine* co, ForeachChildrenCallback callback, void* data);
void coroutine_free(Coroutine* co);
Object call_coroutine(Executor* E, Coroutine* coroutine, Object* arguments, int arguments_count);

#endif