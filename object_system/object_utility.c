#include "object_utility.h"

void set_function(Executor* E, Object t, const char* name, int arguments_count, bool variadic, ObjectSystemFunction f){
    Object function_object;
    function_init(E, &function_object);
    function_object.fp->arguments_count=arguments_count;
    function_object.fp->variadic=variadic;
    function_object.fp->native_pointer=f;
    set(E, t, to_string(name), function_object);
}