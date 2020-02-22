#ifndef OBJECT_UTILITY_H
#define OBJECT_UTILITY_H

#include "object.h"
#include "object_operations.h"

void set_function(Executor* E, Object o, Object key, int minimal_arguments, bool variadic, ObjectSystemFunction native_function);
void set_function_bound(Executor* E, Object o, Object key, int minimal_arguments, bool variadic, ObjectSystemFunction native_function);
Object to_bound_function(Executor* E, Object o, int minimal_arguments, bool variadic, ObjectSystemFunction native_function);

#define BOUND_FUNCTION_CHECK \
    if(arguments[0].type!=t_table || scope.tp!=arguments[0].tp) { \
        RETURN_ERROR("BOUNDED_FUNCTION_ERROR", multiple_causes(E, (Object[]){scope, arguments[0]}, 2), \
        "Object provided to function as first argument is different than the one it was bound to.") \
    }

#define REQUIRE(predicate, cause) if(!(predicate)) { RETURN_ERROR("WRONG_ARGUMENT", cause, "Requirement of function %s wasn't satisfied: %s", __FUNCTION__, #predicate); }
#define REQUIRE_TYPE(o, t) if(o.type!=t) { \
    RETURN_ERROR("WRONG_OBJECT_TYPE", o, "Wrong type of \"%s\" in function %s, it should be %s.", #o, __FUNCTION__, get_type_name(t)); }

#define REQUIRE_ARGUMENT_TYPE(o, t) if(o.type!=t) { \
    RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Wrong type of argument \"%s\" passed to function %s, it should be %s.", #o, __FUNCTION__, get_type_name(t)); }

#define EQUALS_STRING(object, str) (object.type==t_string && strcmp(object.text, str)==0)

#endif