#include "object_utility.h"

Object set_function(Executor* E, Object o, const char* name, int minimal_arguments, bool variadic, ObjectSystemFunction native_function){
    Object function_object;
    function_init(E, &function_object);
    function_object.fp->arguments_count=minimal_arguments;
    if(variadic){
        function_object.fp->arguments_count++;
    }
    function_object.fp->variadic=variadic;
    function_object.fp->native_pointer=native_function;
    return set(E, o, to_string(name), function_object);
}

Object set_function_bound(Executor* E, Object o, char* name, int minimal_arguments, bool variadic, ObjectSystemFunction native_function){
    REQUIRE_ARGUMENT_TYPE(o, t_table)
    Object function=set_function(E, o, name, minimal_arguments, variadic, native_function);
    REQUIRE_TYPE(function, t_function)
    function.fp->enclosing_scope=o;
    reference(&o);
    return function;
}