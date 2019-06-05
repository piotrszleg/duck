#include "pipe_object.h"

object pipe_add(executor* Ex, object* arguments, int arguments_count){
    object pipe=arguments[0];
    object f=arguments[1];
    object count=get(Ex, pipe, to_string("count"));
    if(count.type==t_number){
        USING_STRING(stringify(Ex, count),
            set(Ex, pipe, to_string(str), f));
        count.value++;
        set(Ex, pipe, to_string("count"), count);
    }
    return pipe;
}

object pipe_call(executor* Ex, object* arguments, int arguments_count){
    object pipe=arguments[0];
    object count=get(Ex, pipe, to_string("count"));

    object* subfunction_arguments=arguments+1;
    int subfunction_arguments_count=arguments_count-1;
    object previous_result;

    if(count.type==t_number){
        for(int i=0; i<count.value; i++){
            char buffer[100];
            snprintf(buffer, 100, "%i", i);
            if(i>0) {
                subfunction_arguments=&previous_result;
                subfunction_arguments_count=1;
            }
            previous_result=call(Ex, get(Ex, pipe, to_string(buffer)), subfunction_arguments, subfunction_arguments_count);
        }
    }
    return previous_result;
}

object new_pipe(executor* Ex, object f1, object f2){
    object pipe;
    table_init(&pipe);
    object count;
    number_init(&count);
    count.value=2;
    set(Ex, pipe, to_string("count"), count);
    set(Ex, pipe, to_string("0"), f1);
    set(Ex, pipe, to_string("1"), f2);

    set_function(Ex, pipe, ">>", 2, false, pipe_add);
    set_function(Ex, pipe, "call", 2, true, pipe_call);

    return pipe;
}