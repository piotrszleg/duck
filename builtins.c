#include "builtins.h"

object builtin_print(object* arguments, int arguments_count){
    USING_STRING(stringify(arguments[0]),
        printf("%s\n", str));
    return null_const;
}

object builtin_assert(object* arguments, int arguments_count){
    object self=arguments[0];
    if(is_falsy(self)){
        USING_STRING(stringify(self), 
            ERROR(ASSERTION_FAILED, "Assertion failed, %s is falsy.", str));
    }
    return null_const;
}

object builtin_typeof(object* arguments, int arguments_count){
    object self=arguments[0];
    object type_name;
    string_init(&type_name);
    type_name.text=strdup(OBJECT_TYPE_NAMES[self.type]);
    return type_name;
}

object builtin_native_get(object* arguments, int arguments_count){
    object self=arguments[0];
    object key =arguments[1];
    if(self.type!=t_table){
        ERROR(WRONG_ARGUMENT_TYPE, "Native get function only works on tables. Object type is %s.", OBJECT_TYPE_NAMES[self.type]);
        return null_const;
    }
    return get_table(self.tp, stringify(key));
}

object builtin_native_set(object* arguments, int arguments_count){
    object self =arguments[2];
    object key  =arguments[1]; 
    object value=arguments[2];
    if(self.type!=t_table){
        ERROR(WRONG_ARGUMENT_TYPE, "Native set function only works on tables. Object type is %s.", OBJECT_TYPE_NAMES[self.type]);
        return null_const;
    }
    set_table(self.tp, stringify(key), value);
    return value;
}

object builtin_native_stringify(object* arguments, int arguments_count){
    object result;
    string_init(&result);
    result.text=stringify_object(arguments[0]);
    return result;
}

object builtin_native_call(object* arguments, int arguments_count){
    object self=arguments[0];
    // call function omitting the first argument, because it was function object
    return call(self, arguments+1, arguments_count-1);
}

object builtin_test(object* arguments, int arguments_count){
    for(int i=0; i<arguments_count; i++){
        USING_STRING(stringify_object(arguments[i]),
            printf("<%s>, ", str));
    }
    return null_const;
}

object evaluate_file(const char* file_name, int use_bytecode);
object builtin_include(object* arguments, int arguments_count){
    object path=arguments[0];
    return evaluate_file(stringify(path), true);
}

object evaluate_string(const char* s, bool use_bytecode);
object builtin_eval(object* arguments, int arguments_count){
    object text=arguments[0];
    return evaluate_string(stringify(text), true);
}

void register_builtins(object scope){
    #define REGISTER_FUNCTION(f, args_count) \
        object f##_function; \
        function_init(&f##_function); \
        f##_function.fp->arguments_count=args_count; \
        f##_function.fp->native_pointer=&builtin_##f; \
        set(scope, #f, f##_function);
    
    REGISTER_FUNCTION(print, 1);
    REGISTER_FUNCTION(assert, 1);
    REGISTER_FUNCTION(typeof, 1);
    REGISTER_FUNCTION(native_get, 2);
    REGISTER_FUNCTION(native_call, 2);
    REGISTER_FUNCTION(native_stringify, 1);
    REGISTER_FUNCTION(include, 1);
    REGISTER_FUNCTION(eval, 1);
    //REGISTER_FUNCTION(test, 2);

    #undef REGISTER_FUNCTION
}

object scope_get_override(object* arguments, int arguments_count){
    object self=arguments[0];
    if(self.type!=t_table){
        ERROR(WRONG_ARGUMENT_TYPE, "Table get override incorrect self argument.");
        return null_const;
    }
    object key=arguments[1];
    if(key.type!=t_string){
        ERROR(WRONG_ARGUMENT_TYPE, "Table get override incorrect key argument.");
        return null_const;
    }
    object map_get_result=get_table(self.tp, key.text);

    if(map_get_result.type!=t_null){
        return map_get_result;
    } else{
        object base=get_table(self.tp, "base");
        if(base.type==t_table){
            return get(base, key.text);
        }
        return null_const;
    }
}

void inherit_scope(object scope, object base){
    object f;
    function_init(&f);
    f.fp->native_pointer=&scope_get_override;
    f.fp->arguments_count=2;
    object base_global=get(base, "global");
    if(base_global.type!=t_null){
        set(scope, "global", base_global);
    } else {
        set(scope, "global", base);
        dereference(&base_global);// base_global is null so it can be safely deleted
    }
    set(scope, "get", f);
    set(scope, "scope", scope);
    set(scope, "base", base);
}