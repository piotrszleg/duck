#include "pipe_object.h"

Object pipe_operator(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object pipe=arguments[0];
    Object f=arguments[1];
    Object op=arguments[2];
    REQUIRE_ARGUMENT_TYPE(op, t_string)
    if(strcmp(op.text, "--")==0){
        Object count=get(E, pipe, to_string("count"));
        if(count.type!=t_int) {
            RETURN_ERROR("PIPE_ERROR", pipe, "Count field in pipe object is not a number.");
        }
        set(E, pipe, count, f);
        count.int_value++;
        set(E, pipe, to_string("count"), count);
        return pipe;
    } else {
        OPERATOR_OVERRIDE_FAILURE
    }
}

Object pipe_call(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object pipe=arguments[0];
    Object count=get(E, pipe, to_string("count"));

    Object* subfunction_arguments=arguments+1;
    int subfunction_arguments_count=arguments_count-1;
    Object previous_result;

    if(count.type!=t_int) {
        RETURN_ERROR("PIPE_ERROR", pipe, "Count field in pipe object is not a number.");
    }
    for(int i=0; i<count.int_value; i++){
        char buffer[100];
        snprintf(buffer, 100, "%i", i);
        if(i>0) {
            subfunction_arguments=&previous_result;
            subfunction_arguments_count=1;
        }
        previous_result=call(E, get(E, pipe, to_string(buffer)), subfunction_arguments, subfunction_arguments_count);
    }
    return previous_result;
}

static void add_pipe_fields(Executor* E, Object pipe){
    set(E, pipe, OVERRIDE(E, operator), to_native_function(E, pipe_operator, NULL, 2, false));
    set(E, pipe, OVERRIDE(E, call), to_native_function(E, pipe_call, NULL, 1, true));
}

Object to_pipe(Executor* E, Object f1, Object f2){
    Object pipe;
    table_init(E, &pipe);

    add_pipe_fields(E, pipe);
    set(E, pipe, to_string("count"), to_int(2));
    set(E, pipe, to_string("0"), f1);
    set(E, pipe, to_string("1"), f2);

    return pipe;
}

Object new_pipe(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object pipe;
    table_init(E, &pipe);

    add_pipe_fields(E, pipe);
    set(E, pipe, to_string("count"), to_int(arguments_count));
    for(int i=0; i<arguments_count; i++){
        set(E, pipe, to_int(i), arguments[i]);
    }
    return pipe;
}