#include "pipe_object.h"

Object pipe_operator(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object pipe=arguments[0];
    REQUIRE_ARGUMENT_TYPE(pipe, t_table)
    Object f=arguments[1];
    Object op=arguments[2];
    REQUIRE_ARGUMENT_TYPE(op, t_string)
    if(strcmp(op.text, "--")==0){
        Object count=table_get(E, pipe.tp, to_string("count"));
        if(count.type!=t_int) {
            RETURN_ERROR("PIPE_ERROR", pipe, "Count field in pipe object is not a number.");
        }
        table_set(E, pipe.tp, count, f);
        count.int_value++;
        table_set(E, pipe.tp, to_string("count"), count);
        reference(&pipe);
        return pipe;
    } else {
        OPERATOR_OVERRIDE_FAILURE
    }
}

Object pipe_call(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object pipe=arguments[0];
    REQUIRE_ARGUMENT_TYPE(pipe, t_table)
    Object count=table_get(E, pipe.tp, to_string("count"));

    Object* subfunction_arguments=arguments+1;
    int subfunction_arguments_count=arguments_count-1;
    Object previous_result;

    if(count.type!=t_int) {
        RETURN_ERROR("PIPE_ERROR", pipe, "Count field in pipe object is not a number.");
    }
    for(int i=0; i<count.int_value; i++){
        if(i>0) {
            subfunction_arguments=&previous_result;
            subfunction_arguments_count=1;
        }
        previous_result=call(E, table_get(E, pipe.tp, to_int(i)), subfunction_arguments, subfunction_arguments_count);
        if(i<count.int_value-1){
            dereference(E, &previous_result);
        }
    }
    return previous_result;
}

static void add_pipe_fields(Executor* E, Object pipe){
    table_set(E, pipe.tp, OVERRIDE(E, operator), to_native_function(E, pipe_operator, NULL, 3, false));
    table_set(E, pipe.tp, OVERRIDE(E, call), to_native_function(E, pipe_call, NULL, 1, true));
}

Object to_pipe(Executor* E, Object f1, Object f2){
    Object pipe;
    table_init(E, &pipe);

    add_pipe_fields(E, pipe);
    table_set(E, pipe.tp, to_string("count"), to_int(2));
    table_set(E, pipe.tp, to_int(0), f1);
    table_set(E, pipe.tp, to_int(1), f2);

    return pipe;
}

Object new_pipe(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object pipe;
    table_init(E, &pipe);

    add_pipe_fields(E, pipe);
    table_set(E, pipe.tp, to_string("count"), to_int(arguments_count));
    for(int i=0; i<arguments_count; i++){
        set(E, pipe, to_int(i), arguments[i]);
    }
    return pipe;
}