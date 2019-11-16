#include "patching.h"

Object get_patch(Executor* E, ObjectType object_type, Object patch_symbol) {
    Object patching_table=executor_get_patching_table(E);
    if(patching_table.type==t_null){
        return null_const;
    }
    REQUIRE_TYPE(patching_table, t_table)
    Object type_patching_table=table_get(E, patching_table.tp, get_type_symbol(E, object_type));
    if(type_patching_table.type==t_null){
        return null_const;
    }
    REQUIRE_TYPE(type_patching_table, t_table)
    return table_get(E, type_patching_table.tp, patch_symbol);
}

#define PATCH(patch_name, object_type, ...) \
    Object patch=get_patch(E, object_type, OVERRIDE(E, patch_name)); \
    if(patch.type!=t_null){ \
        Object arguments[]={__VA_ARGS__}; \
        return call(E, patch, arguments, sizeof(arguments)/sizeof(Object)); \
    }
