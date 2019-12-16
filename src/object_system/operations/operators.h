#ifndef OPERATORS_H
#define OPERATORS_H

#include "../object.h"
#include "basic.h"
#include "iteration.h"

ObjectType operator_predict_result(ObjectType a, ObjectType b, const char* op);
Object operator(Executor* E, Object a, Object b, const char* op);

int compare(Executor* E, Object a, Object b);
int compare_and_get_error(Executor* E, Object a, Object b, Object* error);
bool is(Executor* E, Object a, Object b);

#endif