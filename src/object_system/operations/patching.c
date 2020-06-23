#include "patching.h"

Object get_patch(Executor* E, ObjectType object_type, Object patch_symbol) {
    Object type_patching_table=OBJECT_SYSTEM(E)->types_objects[object_type];
    Object result=get_ignore_topmost_prototypes(E, type_patching_table, patch_symbol);
    // if result isn't a function object it is surely added by the user
    if(result.type!=t_function){
        return result;
    }
    Object scope=result.fp->enclosing_scope;
    // ignore functions that aren't added by the user
    if(scope.type==t_symbol && scope.sp==patch_symbol.sp){
        return null_const;
    } else {
        return result;
    }
}