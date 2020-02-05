#include "error_object.h"

char* get_and_stringify(Executor* E, Object t, const char* key){
    Object at_key=get(E, t, to_string(key));
    char* result;
    if(at_key.type!=t_string){
        result=stringify(E, at_key);
    } else {
        result=strdup(at_key.text);
    }
    dereference(E, &at_key);
    return result;
}

Object multiple_causes_stringify(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];

    stream s;
    stream_init(&s, 64);

    Object count_object=get(E, self, to_string("count"));
    REQUIRE_TYPE(count_object, t_int)
    int count=count_object.int_value;

    for(int i=0; i<count; i++){
        Object value=get(E, self, to_int(i));
        char* stringified=stringify(E, value);
        char* indented=string_add("\t", stringified);
        stream_push_string_indented(&s, indented);
        free(stringified);
        free(indented);
        dereference(E, &value);
        stream_push_const_string(&s, "\n");
    }
    stream_push_const_string(&s, "\0");

    return to_string((char*)stream_get_data(&s));
}

Object multiple_causes(Executor* E, Object* causes, int causes_count){
    Object result;
    table_init(E, &result);

    for(int i=0; i<causes_count; i++){
        set(E, result, to_int(i), causes[i]);
    }
    set(E, result, to_string("count"), to_int(causes_count));

    Object stringify_f;
    function_init(E, &stringify_f);
    stringify_f.fp->arguments_count=1;
    stringify_f.fp->native_pointer=multiple_causes_stringify;
    set(E, result, OVERRIDE(E, stringify), stringify_f);
    dereference(E, &stringify_f);

    return result;
}

Object error_stringify(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    char* type=get_and_stringify(E, self, "type");
    char* message=get_and_stringify(E, self, "message");
    char* location=get_and_stringify(E, self, "location");
    char* cause=get_and_stringify(E, self, "cause");
    Object result=to_string(suprintf("%s: %s\n%s\ncaused by:\n%s", type, message, location, cause));
    free(type);
    free(message);
    free(location);
    free(cause);
    set(E, self, to_string("handled"), to_int(1));
    return result;
}

bool is_error(Executor* E, Object o){
    bool result=false;
    if(o.type==t_table){
        Object is_error=get(E, o, OVERRIDE(E, is_error));
        result=is_truthy(is_error);
        dereference(E, &is_error);
    }
    return result;
}

bool is_unhandled_error(Executor* E, Object o){
    bool result=false;
    if(o.type==t_table && is_error(E, o)){
        Object is_handled=get(E, o, to_string("handled"));
        result=is_falsy(is_handled);
        dereference(E, &is_handled);
    }
    return result;
}

void error_handle(Executor* E, Object o){
    Object set_result=set(E, o, to_string("handled"), to_int(1));
    dereference(E, &set_result);
}

void handle_if_error(Executor* E, Object o){
    if(is_error(E, o)){
        error_handle(E, o);
    }
}

Object error_destroy(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    if(is_falsy(get(E, self, to_string("handled")))){
        return executor_on_unhandled_error(E, self);
    } else {
        return null_const;
    }
}

Object new_error(Executor* E, char* type, Object cause, char* message, char* location){
    Object err;
    table_init(E, &err);

    table_set(E, err.tp, to_string("type"), to_string(type));
    table_set(E, err.tp, to_string("message"), to_string(message));
    table_set(E, err.tp, to_string("location"), to_string(location));

    table_set(E, err.tp, OVERRIDE(E, is_error), to_int(1));
    table_set(E, err.tp, to_string("handled"), to_int(0));

    table_set(E, err.tp, OVERRIDE(E, stringify), to_native_function(E, error_stringify, NULL, 1, false));
    table_set(E, err.tp, OVERRIDE(E, destroy), to_native_function(E, error_destroy, NULL, 1, false));

    table_set(E, err.tp, to_string("cause"), cause);

    return err;
}