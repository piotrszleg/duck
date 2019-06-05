#include "binding_object.h"

/*Object binding_bind(Executor* E, Object* arguments, int arguments_count){
    Object binding=arguments[0];
    Object f=arguments[1];
    Object count=get(E, binding, to_string("count"));
    if(count.type==t_number){
        for(int i=0; i<arguments_count-1; i++){
            set(E, binding, to_number(count.value+i), arguments[i+1]);
        }
        count.value++;
        set(E, binding, to_string("count"), count);
    }
    return binding;
}*/

Object binding_bind(Executor* E, Object* arguments, int arguments_count){
    Object binding=arguments[0];
    Object binded_argument=arguments[1];
    Object count=get(E, binding, to_string("count"));

    if(count.type==t_number){
        set(E, binding, to_number(count.value), binded_argument);
        set(E, binding, to_string("count"), to_number(count.value+1));
    }
    return binding;
}

Object binding_bind_multiple(Executor* E, Object* arguments, int arguments_count){
    Object binding=arguments[0];
    Object binded_arguments=arguments[1];
    Object count=get(E, binding, to_string("count"));

    if(count.type==t_number){
        for(int i=0; 1; i++){
            Object argument=get(E, binded_arguments, to_number(i));
            if(argument.type!=t_null){
                set(E, binding, to_number(count.value+i), argument);
            } else {
                set(E, binding, to_string("count"), to_number(count.value+i));
                break;
            }
        }
    }
    return binding;
}

Object binding_call(Executor* E, Object* arguments, int arguments_count){
    Object binding=arguments[0];
    Object count=get(E, binding, to_string("count"));
    Object f=get(E, binding, to_string("f"));

    if(count.type!=t_number) {
        RETURN_ERROR("BINDING_CALL_ERROR", count, "Count field in binding Object is not a number.");
    }
    if(f.type!=t_function){
        RETURN_ERROR("BINDING_CALL_ERROR", count, "Function Object given to binding Object is not a function.");
    }
    int binded_arguments=(int)count.value;
    int total_arguments=binded_arguments+arguments_count-1;
    if(total_arguments==f.fp->arguments_count) {
        Object* concated_arguments=malloc(sizeof(Object)*total_arguments);

        for(int i=0; i<binded_arguments; i++){
            concated_arguments[i]=get(E, binding, to_number(i));
        }
        for(int i=1; i<arguments_count; i++){
            concated_arguments[binded_arguments+i-1]=arguments[i];
        }
        return call(E, f, concated_arguments, total_arguments);
    } else {
        for(int i=0; i<arguments_count; i++){
            set(E, binding, to_number(binded_arguments+i), arguments[i]);
        }
        count.value++;
        set(E, binding, to_string("count"), count);
        return binding;
    }
}

/*Object new_binding(Object f, Object arguments_table){
    Object binding;
    table_init(&binding);
    set(E, binding, to_string("f"), f);

    set_function(binding, "<<", 2, binding_bind);
    set_function(binding, "call", 1, binding_call);
    
    for(int i=0; 1; i++){
        Object argument=get(E, arguments_table, to_number(i));
        if(argument.type!=t_null){
            set(E, binding, to_number(i), argument);
        } else {
            set(E, binding, to_string("count"), to_number(i));
            break;
        }
    }
    return binding;
}*/

Object new_binding(Executor* E, Object f, Object argument){
    Object binding;
    table_init(&binding);

    set_function(E, binding, "<<", 2, false, binding_bind);
    set_function(E, binding, "call", 1, true, binding_call);

    set(E, binding, to_string("f"), f);
    set(E, binding, to_number(0), argument);
    set(E, binding, to_string("count"), to_number(1));
    
    return binding;
}

Object bind_call(Executor* E, Object f, Object* arguments, int arguments_count){
    Object binding;
    table_init(&binding);
    set(E, binding, to_string("f"), f);
    set(E, binding, to_string("count"), to_number(arguments_count));
    
    set_function(E, binding, "<<", 2, false, binding_bind);
    set_function(E, binding, "call", 1, true, binding_call);

    for(int i=0; i<arguments_count; i++){
        set(E, binding, to_number(i), arguments[i]);
    }

    return binding;
}