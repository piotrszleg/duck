#ifndef OPERATORS_H
#define OPERATORS_H

#include "../object.h"
#include "basic.h"
#include "iteration.h"

// it can be safely casted to ObjectType if the value is greater than zero
typedef enum {
    tu_unknown = -1,
    #define X(type) tu_##type=t_##type,
    OBJECT_TYPES
    #undef X
} ObjectTypeOrUnknown;
ObjectTypeOrUnknown operator_predict_result(ObjectTypeOrUnknown a, ObjectTypeOrUnknown b, const char* op);
Object operator(Executor* E, Object a, Object b, const char* op);

int compare(Executor* E, Object a, Object b);
int compare_and_get_error(Executor* E, Object a, Object b, Object* error);
bool is(Executor* E, Object a, Object b);

#endif