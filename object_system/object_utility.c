#include "object_utility.h"

Object set_function(Executor* E, Object o, const char* name, int minimal_arguments, bool variadic, ObjectSystemFunction native_function){
    Object function;
    function_init(E, &function);
    function.fp->arguments_count=minimal_arguments;
    if(variadic){
        function.fp->arguments_count++;
    }
    function.fp->variadic=variadic;
    function.fp->native_pointer=native_function;
    function.fp->ftype=f_native;
    return set(E, o, to_string(name), function);
}

Object to_bound_function(Executor* E, Object o, int minimal_arguments, bool variadic, ObjectSystemFunction native_function){
    REQUIRE_ARGUMENT_TYPE(o, t_table)
    Object function;
    function_init(E, &function);
    function.fp->arguments_count=minimal_arguments;
    if(variadic){
        function.fp->arguments_count++;
    }
    function.fp->enclosing_scope=o;
    function.fp->variadic=variadic;
    function.fp->native_pointer=native_function;
    function.fp->ftype=f_native;
    reference(&o);
    return function;
}

Object set_function_bound(Executor* E, Object o, char* name, int minimal_arguments, bool variadic, ObjectSystemFunction native_function){
    return set(E, o, to_string(name), to_bound_function(E, o, minimal_arguments, variadic, native_function));
}