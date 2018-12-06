#include "builtins.h"

object builtin_print(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(1);
    USING_STRING(stringify(*(object*)vector_get(&arguments, 0)),
        printf("%s\n", str));
    return null_const;
}

object builtin_assert(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(1);
    object* self=vector_get(&arguments, 0);
    if(is_falsy(*self)){
        USING_STRING(stringify(*self), 
            ERROR(ASSERTION_FAILED, "Assertion failed, %s is falsy.", str));
    }
    return null_const;
}

object builtin_typeof(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(1);
    object* self=vector_get(&arguments, 0);
    object type_name;
    string_init(&type_name);
    type_name.text=strdup(OBJECT_TYPE_NAMES[self->type]);
    return type_name;
}

object builtin_native_get(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(2);
    object* self=vector_get(&arguments, 0);
    object* key=vector_get(&arguments, 1);
    if(self->type!=t_table){
        ERROR(WRONG_ARGUMENT_TYPE, "Native get function only works on tables. Object type is %s.", OBJECT_TYPE_NAMES[self->type]);
        return null_const;
    }
    return get_table(self->tp, stringify(*key));
}

object builtin_native_set(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(3);
    object* self=vector_get(&arguments, 0);
    object* key=vector_get(&arguments, 1);
    object* value=vector_get(&arguments, 2);
    if(self->type!=t_table){
        ERROR(WRONG_ARGUMENT_TYPE, "Native set function only works on tables. Object type is %s.", OBJECT_TYPE_NAMES[self->type]);
        return null_const;
    }
    set_table(self->tp, stringify(*key), *value);
    return *value;
}

object builtin_native_stringify(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(1);
    object* self=vector_get(&arguments, 0);
    object result;
    string_init(&result);
    result.text=stringify_object(*self);
    return result;
}

object builtin_native_call(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(2);
    object* self=vector_get(&arguments, 0);
    vector function_arguments;
    vector_init(&function_arguments);
    int arguments_count=vector_total(&arguments);
    for(int i=1; i<arguments_count; i++){
        vector_add(&function_arguments, vector_get(&arguments, i));
    }
    return call(*self, function_arguments);
}

object builtin_test(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(2);
    int arguments_count=vector_total(&arguments);
    for(int i=0; i<arguments_count; i++){
        printf("<%s>, ", stringify(*(object*)vector_get(&arguments, i)));
    }
    return null_const;
}

void register_builtins(object scope){
    #define REGISTER_FUNCTION(f, args_count) \
        object f##_function; \
        function_init(&f##_function); \
        f##_function.fp->arguments_count=args_count; \
        f##_function.fp->pointer=&builtin_##f; \
        set(scope, #f, f##_function);
    
    REGISTER_FUNCTION(print, 1);
    REGISTER_FUNCTION(assert, 1);
    REGISTER_FUNCTION(typeof, 1);
    REGISTER_FUNCTION(native_get, 2);
    REGISTER_FUNCTION(native_call, 2);
    REGISTER_FUNCTION(native_stringify, 1);
    //REGISTER_FUNCTION(test, 2);

    #undef REGISTER_FUNCTION
}

object scope_get_override(vector arguments){
    BUILTIN_ARGUMENTS_CHECK(2);
    object* self=vector_get(&arguments, 0);
    if(self->type!=t_table){
        ERROR(WRONG_ARGUMENT_TYPE, "Table get override incorrect self argument.");
        return null_const;
    }
    object* key=vector_get(&arguments, 1);
    if(key->type!=t_string){
        ERROR(WRONG_ARGUMENT_TYPE, "Table get override incorrect key argument.");
        return null_const;
    }
    object map_get_result=get_table(self->tp, key->text);

    if(map_get_result.type!=t_null){
        return map_get_result;
    } else{
        object base=get_table(self->tp, "base");
        if(base.type==t_table){
            return get(base, key->text);
        }
        return null_const;
    }
}

void inherit_scope(object scope, object base){
    object f;
    function_init(&f);
    f.fp->pointer=&scope_get_override;
    f.fp->arguments_count=2;
    object base_global=get(base, "global");
    if(base_global.type!=t_null){
        set(scope, "global", base_global);
    } else {
        set(scope, "global", base);
        object_deinit(&base_global);// base_global is null so it can be safely deleted
    }
    set(scope, "get", f);
    set(scope, "scope", scope);
    set(scope, "base", base);
}