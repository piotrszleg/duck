#ifndef PATCHING_H
#define PATCHING_H

#include "../object.h"
#include "../object_utility.h"

#define OBJECTS_ARRAY(...) ((Object[]){__VA_ARGS__})

Object get_patch(Executor* E, ObjectType object_type, Object patch_symbol);

// calls patched function (if it exists) with arguments provided as variadic arguments to object of type
#define PATCH(patch_symbol, object_type, ...) \
    Object patch=get_patch(E, object_type, OVERRIDE(E, patch_symbol)); \
    if(patch.type!=t_null){ \
        Object arguments[]={__VA_ARGS__}; \
        Object result=call(E, patch, arguments, sizeof(arguments)/sizeof(Object)); \
        dereference(E, &patch); \
        return result; \
    }

#endif