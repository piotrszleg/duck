#ifndef PATCHING_H
#define PATCHING_H

#include "../object.h"
#include "../object_utility.h"

#define OBJECTS_ARRAY(...) ((Object[]){__VA_ARGS__})

Object get_patch(Executor* E, ObjectType object_type, Object patch_symbol);

#define PATCH(patch_name, object_type, ...) \
    Object patch=get_patch(E, object_type, OVERRIDE(E, patch_name)); \
    if(patch.type!=t_null){ \
        Object arguments[]={__VA_ARGS__}; \
        return call(E, patch, arguments, sizeof(arguments)/sizeof(Object)); \
    }

#endif