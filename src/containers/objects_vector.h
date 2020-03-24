#ifndef OBJECTS_VECTOR_VECTOR_H
#define OBJECTS_VECTOR_VECTOR_H

#include "vector.h"
#include "../object_system/object.h"
#include "stdbool.h"

void objects_vector_push(vector* stack, Object object);
Object objects_vector_pop(vector* stack);
bool objects_vector_delete(Executor* E, vector* v, Object o);

#endif