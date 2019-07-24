#include "binding_object.h"

Object binding_bind_operator(Executor* E, Object* arguments, int arguments_count){
    Object binding=arguments[0];
    Object binded_argument=arguments[1];
    Object count=get(E, binding, to_string("count"));

    if(count.type==t_int){
        set(E, binding, count, binded_argument);
        count.int_value++;
        set(E, binding, to_string("count"), count);
    }
    return binding;
}

Object binding_bind(Executor* E, Object* arguments, int arguments_count){
    Object binding=arguments[0];
    Object count=get(E, binding, to_string("count"));

    if(count.type!=t_int) {
        RETURN_ERROR("BINDING_ERROR", count, "Count field in binding Object is not a number.");
    }

    for(int i=1; i<arguments_count; i++){
        set(E, binding, to_int(count.int_value+i), arguments[i]);
    }
    return binding;
}

Object binding_call(Executor* E, Object* arguments, int arguments_count){
    Object binding=arguments[0];
    Object count=get(E, binding, to_string("count"));
    Object f=get(E, binding, to_string("f"));

    if(count.type!=t_int) {
        RETURN_ERROR("BINDING_ERROR", count, "Count field in binding Object is not a number.");
    }
    int binded_arguments=(int)count.int_value;
    int total_arguments=binded_arguments+arguments_count-1;

    Object* concated_arguments=malloc(sizeof(Object)*total_arguments);
    for(int i=0; i<binded_arguments; i++){
        concated_arguments[i]=get(E, binding, to_int(i));
    }
    for(int i=1; i<arguments_count; i++){
        concated_arguments[binded_arguments+i-1]=arguments[i];
    }
    return call(E, f, concated_arguments, total_arguments);
}

void add_binding_fields(Executor* E, Object binding){
    set_function(E, binding, "><", 2, false, binding_bind_operator);
    set_function(E, binding, "call", 1, true, binding_call);
    set_function(E, binding, "bind", 1, true, binding_bind);
}

Object to_binding(Executor* E, Object f, Object argument){
    Object binding;
    table_init(E, &binding);

    add_binding_fields(E, binding);
    set(E, binding, to_string("f"), f);
    set(E, binding, to_int(0), argument);
    set(E, binding, to_string("count"), to_int(1));
    
    return binding;
}

Object new_binding(Executor* E, Object* arguments, int arguments_count){
    Object binding;
    table_init(E, &binding);

    add_binding_fields(E, binding);
    set(E, binding, to_string("f"), arguments[0]);
    set(E, binding, to_string("count"), to_int(arguments_count-1));
    for(int i=0; i<arguments_count-1; i++){
        set(E, binding, to_int(i), arguments[i+1]);
    }

    return binding;
}