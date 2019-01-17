#include "error_object.h"

void set_string_field(object t, const char* field_name, char* string){
    object string_object;
    string_init(&string_object);
    string_object.text=string;
    set(t, field_name, string_object);
    dereference(&string_object);
}

void set_number_field(object t, const char* field_name, int value){
    object number_object;
    number_init(&number_object);
    number_object.value=value;
    set(t, field_name, number_object);
    dereference(&number_object);
}

char* get_and_stringify(object t, const char* key){
    object at_key=get(t, key);
    return stringify(at_key);
}

object stringify_multiple_causes(object* arguments, int arguments_count){
    char* buffer=malloc(STRINGIFY_BUFFER_SIZE*sizeof(char));
    buffer[0]='\0';
    object self=arguments[0];

    object count_object=get(self, "count");
    //assert(count_object.type==t_number);
    int count=count_object.value;
    dereference(&count_object);

    for(int i=0; i<count; i++){
        char stringified_key[64];
        snprintf(stringified_key, 64, "%i", i);

        object value=get(self, stringified_key);
        char* stringified_value=stringify(value);

        char formatted[STRINGIFY_BUFFER_SIZE];
        sprintf(formatted, "(%i/%i) %s\n", i+1, count, stringified_value);
        strncat(buffer, formatted, STRINGIFY_BUFFER_SIZE);

        free(stringified_value);
    }
    object result;
    string_init(&result);
    result.text=buffer;
    return result;
}

object multiple_causes(object* causes, int causes_count){
    object result;
    table_init(&result);

    for(int i=0; i<causes_count; i++){
        char buffer[64];
        snprintf(buffer, 64, "%i", i);
        set(result, buffer, causes[i]);
    }
    
    object count;
    number_init(&count);
    count.value=causes_count;
    set(result, "count", count);

    object stringify_f;
    function_init(&stringify_f);
    stringify_f.fp->arguments_count=1;
    stringify_f.fp->native_pointer=stringify_multiple_causes;
    set(result, "stringify", stringify_f);

    return result;
}

object stringify_error(object* arguments, int arguments_count){
    char* buffer=malloc(STRINGIFY_BUFFER_SIZE*sizeof(char));
    object self=arguments[0];
    char* type=get_and_stringify(self, "type");
    char* message=get_and_stringify(self, "message");
    char* location=get_and_stringify(self, "location");
    char* cause=get_and_stringify(self, "cause");
    snprintf(buffer, STRINGIFY_BUFFER_SIZE, "%s: %s\n%s\ncaused by:\n%s", type, message, location, cause);
    free(type);
    free(message);
    free(location);
    free(cause);
    object result;
    string_init(&result);
    result.text=buffer;
    set_number_field(self, "handled", 1);
    return result;
}

object destroy_error(object* arguments, int arguments_count){
    object self=arguments[0];
    if(is_falsy(get(self, "handled"))){
        USING_STRING(stringify(self),
            printf("Unhandled error:\n%s", str));
    }
    return null_const;
}

void set_function(object t, const char* name, int arguments_count, object_system_function f);

object new_error(char* type, object cause, char* message, char* location){
    object err;
    table_init(&err);

    set_string_field(err, "type", type);
    set_string_field(err, "message", message);
    set_string_field(err, "location", location);

    set_number_field(err, "error", 1);

    set_function(err, "stringify", 1, stringify_error);
    set_function(err, "destroy", 1, destroy_error);

    set(err, "cause", cause);

    return err;
}