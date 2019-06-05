#include "error_object.h"

char* get_and_stringify(Executor* E, Object t, const char* key){
    Object at_key=get(E, t, to_string(key));
    return stringify(E, at_key);
}

Object stringify_multiple_causes(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];

    stream s;
    init_stream(&s, 64);

    Object count_object=get(E, self, to_string("count"));
    REQUIRE_TYPE(count_object, t_number)
    int count=count_object.value;

    for(int i=0; i<count; i++){
        char stringified_key[64];
        snprintf(stringified_key, 64, "%i", i);

        Object value=get(E, self, to_string(stringified_key));
        char* stringified_value=stringify(E, value);

        char* formatted=suprintf("(%i/%i) %s\n", i+1, count, stringified_value);
        stream_push(&s, formatted, strlen(formatted));

        free(formatted);
        free(stringified_value);
    }
    stream_push(&s, "\0", 1);

    Object result;
    string_init(&result);
    result.text=(char*)stream_get_data(&s);
    return result;
}

Object multiple_causes(Executor* E, Object* causes, int causes_count){
    Object result;
    table_init(&result);

    for(int i=0; i<causes_count; i++){
        char buffer[64];
        snprintf(buffer, 64, "%i", i);
        set(E, result, to_string(buffer), causes[i]);
    }
    
    Object count;
    number_init(&count);
    count.value=causes_count;
    set(E, result, to_string("count"), count);

    Object stringify_f;
    function_init(&stringify_f);
    stringify_f.fp->arguments_count=1;
    stringify_f.fp->native_pointer=stringify_multiple_causes;
    set(E, result, to_string("stringify"), stringify_f);

    return result;
    return null_const;
}

Object stringify_error(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    char* type=get_and_stringify(E, self, "type");
    char* message=get_and_stringify(E, self, "message");
    char* location=get_and_stringify(E, self, "location");
    char* cause=get_and_stringify(E, self, "cause");
    Object result;
    string_init(&result);
    result.text=suprintf("%s: %s\n%s\ncaused by:\n%s", type, message, location, cause);
    free(type);
    free(message);
    free(location);
    free(cause);
    set(E, self, to_string("handled"), to_number(1));
    return result;
}

Object destroy_error(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    if(is_falsy(get(E, self, to_string("handled")))){
        USING_STRING(stringify(E, self),
            printf("Unhandled error:\n%s", str));
    }
    return null_const;
}

Object new_error(Executor* E, char* type, Object cause, char* message, char* location){
    Object err;
    table_init(&err);

    set(E, err, to_string("type"), to_string(type));
    set(E, err, to_string("message"), to_string(message));
    set(E, err, to_string("location"), to_string(location));

    set(E, err, to_string("error"), to_number(1));
    set(E, err, to_string("handled"), to_number(0));

    set(E, err, to_string("stringify"), to_function(stringify_error, NULL, 1));
    set(E, err, to_string("destroy"), to_function(destroy_error, NULL, 1));

    set(E, err, to_string("cause"), cause);

    return err;
    return null_const;
}