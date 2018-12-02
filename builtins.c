#include "builtins.h"

object* builtin_print(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(1);
    USING_STRING(stringify(vector_get(&arguments, 0)),
        printf("%s\n", str));
    return (object*)new_null();
}

object* builtin_assert(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(1);
    object* self=vector_get(&arguments, 0);
    if(is_falsy(self)){
        USING_STRING(stringify(self), 
            ERROR(ASSERTION_FAILED, "Assertion failed, %s is falsy.", str));
    }
    return (object*)new_null();
}

object* builtin_typeof(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(1);
    object* self=vector_get(&arguments, 0);
    string* type_name=new_string();
    type_name->value=strdup(OBJECT_TYPE_NAMES[self->type]);
    return (object*)type_name;
}

object* native_get(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(2);
    object* self=vector_get(&arguments, 0);
    object* key=vector_get(&arguments, 1);
    return get(self, stringify(key));
}

object* builtin_native_call(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(2);
    object* self=vector_get(&arguments, 0);
    vector function_arguments;
    vector_init(&function_arguments);
    int arguments_count=vector_total(&arguments);
    for(int i=1; i<arguments_count; i++){
        vector_add(&function_arguments, vector_get(&arguments, i));
    }
    return call(self, function_arguments);
}

object* builtin_test(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(2);
    int arguments_count=vector_total(&arguments);
    for(int i=0; i<arguments_count; i++){
        printf("<%s>, ", stringify(vector_get(&arguments, i)));
    }
    return (object*)new_null();
}

void register_builtins(table* scope){
    #define REGISTER_FUNCTION(f, args_count) \
        function* f##_function=new_function(); \
        f##_function->arguments_count=args_count; \
        f##_function->pointer=&builtin_##f; \
        set((object*)scope, #f, (object*)f##_function);
    
    REGISTER_FUNCTION(print, 1);
    REGISTER_FUNCTION(assert, 1);
    REGISTER_FUNCTION(typeof, 1);
    REGISTER_FUNCTION(native_call, 2);
    //REGISTER_FUNCTION(test, 2);
}

object* scope_get_override(vector arguments){
    object* self=vector_get(&arguments, 0);
    if(self->type!=t_table){
        ERROR(WRONG_ARGUMENT_TYPE, "Table get override incorrect self argument.");
        return (object*)new_null();
    }
    object* key=vector_get(&arguments, 1);
    if(key->type!=t_string){
        ERROR(WRONG_ARGUMENT_TYPE, "Table get override incorrect key argument.");
        return (object*)new_null();
    }
    object** map_get_result=map_get(&((table*)self)->fields, ((string*)key)->value);

    if(map_get_result!=NULL){
        return *map_get_result;
    } else{
        object* base=get((object*)self, "base");
        if(base->type==t_table){
            return get(base, ((string*)key)->value);
        }
        return (object*)new_null();
    }
}

void inherit_scope(object* scope, object* base){
    function* f=new_function();
    f->pointer=&scope_get_override;
    object* base_global=get(base, "global");
    if(base_global->type!=t_null){
        set(scope, "global", base_global);
    } else {
        set(scope, "global", base);
        object_delete(base_global);// base_global is null so it can be safely deleted
    }
    set(scope, "get", (object*)f);
    set(scope, "scope", scope);
    set(scope, "base", base);
}