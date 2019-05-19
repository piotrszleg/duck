#include "object_utility.h"

void set_function(object t, const char* name, int arguments_count, bool variadic, object_system_function f){
    object function_object;
    function_init(&function_object);
    function_object.fp->arguments_count=arguments_count;
    function_object.fp->variadic=variadic;
    function_object.fp->native_pointer=f;
    set(t, to_string(name), function_object);
}