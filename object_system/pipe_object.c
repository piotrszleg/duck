#include "pipe_object.h"

object pipe_add(object* arguments, int arguments_count){
    object pipe=arguments[0];
    object f=arguments[1];
    object count=get(pipe, "count");
    if(count.type==t_number){
        USING_STRING(stringify(count),
            set(pipe, str, f));
        count.value++;
        set(pipe, "count", count);
    }
    return null_const;
}

object pipe_call(object* arguments, int arguments_count){
    object pipe=arguments[0];
    object count=get(pipe, "count");

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
            previous_result=call(get(pipe, buffer), subfunction_arguments, subfunction_arguments_count);
        }
    }
    return previous_result;
}

void set_function(object t, const char* name, int arguments_count, object_system_function f){
    object function_object;
    function_init(&function_object);
    function_object.fp->arguments_count=arguments_count;
    function_object.fp->native_pointer=f;
    set(t, name, function_object);
}

object new_pipe(object f1, object f2){
    object pipe;
    table_init(&pipe);
    object count;
    number_init(&count);
    count.value=2;
    set(pipe, "count", count);
    set(pipe, "0", f1);
    set(pipe, "1", f2);

    set_function(pipe, "--", 2, pipe_add);
    set_function(pipe, "call", 1, pipe_call);

    return pipe;
}