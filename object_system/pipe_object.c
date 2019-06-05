#include "pipe_object.h"

Object pipe_add(Executor* E, Object* arguments, int arguments_count){
    Object pipe=arguments[0];
    Object f=arguments[1];
    Object count=get(E, pipe, to_string("count"));
    if(count.type==t_number){
        USING_STRING(stringify(E, count),
            set(E, pipe, to_string(str), f));
        count.value++;
        set(E, pipe, to_string("count"), count);
    }
    return pipe;
}

Object pipe_call(Executor* E, Object* arguments, int arguments_count){
    Object pipe=arguments[0];
    Object count=get(E, pipe, to_string("count"));

    Object* subfunction_arguments=arguments+1;
    int subfunction_arguments_count=arguments_count-1;
    Object previous_result;

    if(count.type==t_number){
        for(int i=0; i<count.value; i++){
            char buffer[100];
            snprintf(buffer, 100, "%i", i);
            if(i>0) {
                subfunction_arguments=&previous_result;
                subfunction_arguments_count=1;
            }
            previous_result=call(E, get(E, pipe, to_string(buffer)), subfunction_arguments, subfunction_arguments_count);
        }
    }
    return previous_result;
}

Object new_pipe(Executor* E, Object f1, Object f2){
    Object pipe;
    table_init(&pipe);
    Object count;
    number_init(&count);
    count.value=2;
    set(E, pipe, to_string("count"), count);
    set(E, pipe, to_string("0"), f1);
    set(E, pipe, to_string("1"), f2);

    set_function(E, pipe, ">>", 2, false, pipe_add);
    set_function(E, pipe, "call", 2, true, pipe_call);

    return pipe;
}