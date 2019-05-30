#include "error_object.h"

char* get_and_stringify(object t, const char* key){
    object at_key=get(t, to_string(key));
    return stringify(at_key);
}

object stringify_multiple_causes(object* arguments, int arguments_count){
    object self=arguments[0];

    stream s;
    init_stream(&s, 64);

    object count_object=get(self, to_string("count"));
    REQUIRE_TYPE(count_object, t_number)
    int count=count_object.value;

    for(int i=0; i<count; i++){
        char stringified_key[64];
        snprintf(stringified_key, 64, "%i", i);

        object value=get(self, to_string(stringified_key));
        char* stringified_value=stringify(value);

        char* formatted=suprintf("(%i/%i) %s\n", i+1, count, stringified_value);
        stream_push(&s, formatted, strlen(formatted));

        free(formatted);
        free(stringified_value);
    }
    stream_push(&s, "\0", 1);

    object result;
    string_init(&result);
    result.text=(char*)stream_get_data(&s);
    return result;
}

object multiple_causes(object* causes, int causes_count){
    object result;
    table_init(&result);

    for(int i=0; i<causes_count; i++){
        char buffer[64];
        snprintf(buffer, 64, "%i", i);
        set(result, to_string(buffer), causes[i]);
    }
    
    object count;
    number_init(&count);
    count.value=causes_count;
    set(result, to_string("count"), count);

    object stringify_f;
    function_init(&stringify_f);
    stringify_f.fp->arguments_count=1;
    stringify_f.fp->native_pointer=stringify_multiple_causes;
    set(result, to_string("stringify"), stringify_f);

    return result;
    return null_const;
}

object stringify_error(object* arguments, int arguments_count){
    object self=arguments[0];
    char* type=get_and_stringify(self, "type");
    char* message=get_and_stringify(self, "message");
    char* location=get_and_stringify(self, "location");
    char* cause=get_and_stringify(self, "cause");
    object result;
    string_init(&result);
    result.text=suprintf("%s: %s\n%s\ncaused by:\n%s", type, message, location, cause);
    free(type);
    free(message);
    free(location);
    free(cause);
    set(self, to_string("handled"), to_number(1));
    return result;
}

object destroy_error(object* arguments, int arguments_count){
    object self=arguments[0];
    if(is_falsy(get(self, to_string("handled")))){
        USING_STRING(stringify(self),
            printf("Unhandled error:\n%s", str));
    }
    return null_const;
}

object new_error(char* type, object cause, char* message, char* location){
    object err;
    table_init(&err);

    set(err, to_string("type"), to_string(type));
    set(err, to_string("message"), to_string(message));
    set(err, to_string("location"), to_string(location));

    set(err, to_string("error"), to_number(1));
    set(err, to_string("handled"), to_number(0));

    set(err, to_string("stringify"), to_function(stringify_error, NULL, 1));
    set(err, to_string("destroy"), to_function(destroy_error, NULL, 1));

    set(err, to_string("cause"), cause);

    return err;
    return null_const;
}