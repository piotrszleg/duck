#include "binding_object.h"

Object binding_operator(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object binding=arguments[0];
    REQUIRE_ARGUMENT_TYPE(binding, t_table)
    Object binded_argument=arguments[1];
    Object op=arguments[2];
    Object count=table_get(E, binding.tp, to_string("count"));
    REQUIRE_TYPE(count, t_int)
    if(strcmp(op.text, "><")==0){
        table_set(E, binding.tp, count, binded_argument);
        count.int_value++;
        table_set(E, binding.tp, to_string("count"), count);
    } else {
        OPERATOR_OVERRIDE_FAILURE
    }
    reference(&binding);
    return binding;
}

Object binding_bind(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object binding=arguments[0];
    REQUIRE_ARGUMENT_TYPE(binding, t_table)
    Object count=table_get(E, binding.tp, to_string("count"));

    if(count.type!=t_int) {
        RETURN_ERROR("BINDING_ERROR", count, "Count field in binding object is not a number.");
    }

    for(int i=1; i<arguments_count; i++){
        table_set(E, binding.tp, to_int(count.int_value+i), arguments[i]);
    }
    reference(&binding);
    return binding;
}

Object binding_call(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object binding=arguments[0];
    REQUIRE_ARGUMENT_TYPE(binding, t_table)
    Object count=table_get(E, binding.tp, to_string("count"));
    Object f=table_get(E, binding.tp, to_string("f"));

    if(count.type!=t_int) {
        RETURN_ERROR("BINDING_ERROR", count, "Count field in binding object is not a number.");
    }
    int binded_arguments=(int)count.int_value;
    int total_arguments=binded_arguments+arguments_count-1;

    Object* concated_arguments=malloc(sizeof(Object)*total_arguments);
    for(int i=0; i<binded_arguments; i++){
        concated_arguments[i]=table_get(E, binding.tp, to_int(i));
    }
    for(int i=1; i<arguments_count; i++){
        concated_arguments[binded_arguments+i-1]=arguments[i];
    }
    return call(E, f, concated_arguments, total_arguments);
}

void add_binding_fields(Executor* E, Object binding){
    table_set(E, binding.tp, OVERRIDE(E, operator), to_native_function(E,  binding_operator, NULL, 3, false));
    table_set(E, binding.tp, OVERRIDE(E, call), to_native_function(E, binding_call, NULL, 1, true));
    set_function(E, binding, to_string("bind"), 1, true, binding_bind);
}

Object to_binding(Executor* E, Object f, Object argument){
    Object binding;
    table_init(E, &binding);

    add_binding_fields(E, binding);
    table_set(E, binding.tp, to_string("f"), f);
    table_set(E, binding.tp, to_int(0), argument);
    table_set(E, binding.tp, to_string("count"), to_int(1));
    
    return binding;
}

Object new_binding(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object binding;
    table_init(E, &binding);

    add_binding_fields(E, binding);
    table_set(E, binding.tp, to_string("f"), arguments[0]);
    table_set(E, binding.tp, to_string("count"), to_int(arguments_count-1));
    for(int i=0; i<arguments_count-1; i++){
        table_set(E, binding.tp, to_int(i), arguments[i+1]);
    }

    return binding;
}