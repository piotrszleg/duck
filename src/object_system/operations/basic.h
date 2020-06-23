#ifndef BASIC_H
#define BASIC_H

#include "../object.h"
#include "copying_state.h"
#include "patching.h"
#include "../../utility.h"
#include "../../containers/map.h"
#include "../../c_fixes.h"
#include <stdbool.h>

bool is_falsy(Object o);
bool is_truthy(Object o);

uint hash_and_get_error(Executor* E, Object o, Object* error);
uint hash(Executor* E, Object o);

#define OPERATOR_OVERRIDE_FAILURE \
    RETURN_ERROR("OPERATOR_ERROR", multiple_causes(E, OBJECTS_ARRAY(arguments[0], arguments[1]), 2), \
    "Can't perform operotion '%s' on objects of type <%s> and <%s>", arguments[2].text, get_type_name(arguments[0].type), get_type_name(arguments[1].type));

bool cast_is_constant(ObjectType from, ObjectType to);
Object cast(Executor* E, Object o, ObjectType type);
Object call(Executor* E, Object o, Object* arguments, int arguments_count);
Object get(Executor* E, Object o, Object key);
Object get_ignore_topmost_prototypes(Executor* E, Object o, Object key);
Object set(Executor* E, Object o, Object key, Object value);
Object get_prototype(Executor* E, Object o);

void get_execution_info(Executor* E, char* buffer, int buffer_count);

#include "../special_objects/error_object.h"
#include "../special_objects/binding_object.h"
#include "../special_objects/pipe_object.h"

#endif