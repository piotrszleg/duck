#include "object_utility.h"

void set_function(Executor* E, Object o, Object key, int minimal_arguments, bool variadic, ObjectSystemFunction native_function){
    Object function;
    function_init(E, &function);
    function.fp->arguments_count=minimal_arguments;
    if(variadic){
        function.fp->arguments_count++;
    }
    function.fp->variadic=variadic;
    function.fp->native_pointer=native_function;
    function.fp->ftype=f_native;
    table_set(E, o.tp, key, function);
    dereference(E, &function);
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

void set_function_bound(Executor* E, Object o, Object key, int minimal_arguments, bool variadic, ObjectSystemFunction native_function){
    Object function=to_bound_function(E, o, minimal_arguments, variadic, native_function);
    table_set(E, o.tp, key, function);
    dereference(E, &function);
}