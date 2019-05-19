#include "binding_object.h"

/*object binding_bind(object* arguments, int arguments_count){
    object binding=arguments[0];
    object f=arguments[1];
    object count=get(binding, to_string("count"));
    if(count.type==t_number){
        for(int i=0; i<arguments_count-1; i++){
            set(binding, to_number(count.value+i), arguments[i+1]);
        }
        count.value++;
        set(binding, to_string("count"), count);
    }
    return binding;
}*/

object binding_bind(object* arguments, int arguments_count){
    object binding=arguments[0];
    object binded_argument=arguments[1];
    object count=get(binding, to_string("count"));

    if(count.type==t_number){
        set(binding, to_number(count.value), binded_argument);
        set(binding, to_string("count"), to_number(count.value+1));
    }
    return binding;
}

object binding_bind_multiple(object* arguments, int arguments_count){
    object binding=arguments[0];
    object binded_arguments=arguments[1];
    object count=get(binding, to_string("count"));

    if(count.type==t_number){
        for(int i=0; 1; i++){
            object argument=get(binded_arguments, to_number(i));
            if(argument.type!=t_null){
                set(binding, to_number(count.value+i), argument);
            } else {
                set(binding, to_string("count"), to_number(count.value+i));
                break;
            }
        }
    }
    return binding;
}

object binding_call(object* arguments, int arguments_count){
    object binding=arguments[0];
    object count=get(binding, to_string("count"));
    object f=get(binding, to_string("f"));

    if(count.type!=t_number) {
        RETURN_ERROR("BINDING_CALL_ERROR", count, "Count field in binding object is not a number.");
    }
    if(f.type!=t_function){
        RETURN_ERROR("BINDING_CALL_ERROR", count, "Function object given to binding object is not a function.");
    }
    if(count.type==t_number){
        int binded_arguments=(int)count.value;
        int total_arguments=binded_arguments+arguments_count-1;
        if(total_arguments==f.fp->arguments_count) {
            object* concated_arguments=malloc(sizeof(object)*total_arguments);

            for(int i=0; i<binded_arguments; i++){
                concated_arguments[i]=get(binding, to_number(i));
            }
            for(int i=1; i<arguments_count; i++){
                concated_arguments[binded_arguments+i-1]=arguments[i];
            }
            return call(f, concated_arguments, total_arguments);
        } else {
            for(int i=0; i<arguments_count; i++){
                set(binding, to_number(binded_arguments+i), arguments[i]);
            }
            count.value++;
            set(binding, to_string("count"), count);
            return binding;
        }
    }
}

/*object new_binding(object f, object arguments_table){
    object binding;
    table_init(&binding);
    set(binding, to_string("f"), f);

    set_function(binding, "<<", 2, binding_bind);
    set_function(binding, "call", 1, binding_call);
    
    for(int i=0; 1; i++){
        object argument=get(arguments_table, to_number(i));
        if(argument.type!=t_null){
            set(binding, to_number(i), argument);
        } else {
            set(binding, to_string("count"), to_number(i));
            break;
        }
    }
    return binding;
}*/

object new_binding(object f, object argument){
    object binding;
    table_init(&binding);

    set_function(binding, "<<", 2, false, binding_bind);
    set_function(binding, "call", 1, true, binding_call);

    set(binding, to_string("f"), f);
    set(binding, to_number(0), argument);
    set(binding, to_string("count"), to_number(1));
    
    return binding;
}

object bind_call(object f, object* arguments, int arguments_count){
    object binding;
    table_init(&binding);
    set(binding, to_string("f"), f);
    set(binding, to_string("count"), to_number(arguments_count));
    
    set_function(binding, "<<", 2, false, binding_bind);
    set_function(binding, "call", 1, true, binding_call);

    for(int i=0; i<arguments_count; i++){
        set(binding, to_number(i), arguments[i]);
    }

    return binding;
}