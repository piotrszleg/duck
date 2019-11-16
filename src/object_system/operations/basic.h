#ifndef BASIC_H
#define BASIC_H

#include "../object.h"
#include "patching.h"
#include "../table.h"
#include "../../utility.h"
#include "../../containers/map.h"
#include <stdbool.h>

bool is_falsy(Object o);
bool is_truthy(Object o);

unsigned hash(Executor* E, Object o, Object* error);

#define OPERATOR_OVERRIDE_FAILURE \
    RETURN_ERROR("OPERATOR_ERROR", multiple_causes(E, OBJECTS_ARRAY(arguments[0], arguments[1]), 2), \
    "Can't perform operotion '%s' on objects of type <%s> and <%s>", arguments[2].text, get_type_name(arguments[0].type), get_type_name(arguments[1].type));

bool cast_is_constant(ObjectType from, ObjectType to);
Object cast(Executor* E, Object o, ObjectType type);
Object call(Executor* E, Object o, Object* arguments, int arguments_count);
Object get(Executor* E, Object o, Object key);
Object set(Executor* E, Object o, Object key, Object value);

void get_execution_info(Executor* E, char* buffer, int buffer_count);

Object copy_recursive(Executor* E, Object o, Table* copies);
Object copy(Executor* E, Object o);

#include "../special_objects/error_object.h"
#include "../special_objects/binding_object.h"
#include "../special_objects/pipe_object.h"

#endif