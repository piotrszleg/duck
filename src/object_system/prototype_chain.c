#include "prototype_chain.h"

// TOFIX: this doesn't comply with call override convention
Object any_call(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object function=arguments[0];
    Object arguments_table=arguments[1];
    int i=0;
    Object object_at_i;
    do{
        object_at_i=get(E, arguments_table, to_int(i));
        i++;
    } while(object_at_i.type!=t_null);
    Object* arguments_array=malloc(sizeof(Object)*i);
    for(int j=0; j<i; j++){
        arguments_array[j]=get(E, arguments_table, to_int(j));
    }
    Object result=call(E, function, arguments_array, i);
    free(arguments_array);
    return result;

    if(arguments[0].type==t_table){
        return call(E, function, arguments_array, i);
    } else {
        return call(E, function, arguments_array, i);
    }
}

// TOFIX: this two will fall into infinite recursion on a specially prepared table
Object any_operator(Executor* E, Object scope, Object* arguments, int arguments_count){
    REQUIRE_ARGUMENT_TYPE(arguments[2], t_string)
    return operator(E, arguments[0], arguments[1], arguments[2].text);
}

Object any_cast(Executor* E, Object scope, Object* arguments, int arguments_count){
    for(int i=0; i<=LAST_OBJECT_TYPE; i++){
        if(compare(E, get_type_symbol(E, i), arguments[1])==0){
            return cast(E, arguments[0], i);
        }
    }
    RETURN_ERROR("TYPE_CONVERSION_FAILURE", MULTIPLE_CAUSES(arguments[0], arguments[1]), "Any cast failed.");
}

Object any_compare(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object error=null_const;
    int result;
    if(arguments[0].type==t_table){
        if(arguments[1].type!=t_table){
            RETURN_ERROR("COMPARISON_ERROR", MULTIPLE_CAUSES(arguments[0], arguments[1]), "Can't compare objects because their type is different.")
        }
        result=table_compare(E, arguments[0].tp, arguments[1].tp, &error);
    } else {
        result=compare_and_get_error(E, arguments[0], arguments[1], &error);
    }
    if(error.type!=t_null){
        return error;
    } else {
        return to_int(result);
    }
}

Object any_hash(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object error=null_const;
    int result;
    if(arguments[0].type==t_table){
        result=table_hash(E, arguments[0].tp, &error);
    }
    else{
        result=hash_and_get_error(E, arguments[0], &error);
    }
    if(error.type!=t_null){
        return error;
    } else {
        return to_int(result);
    }
}

Object any_iterator(Executor* E, Object scope, Object* arguments, int arguments_count){
    if(arguments[0].type==t_table){
        return table_get_iterator_object(E, scope, arguments, arguments_count);
    } else {
        return get_iterator(E, arguments[0]);
    }
}

Object any_stringify(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object object=arguments[0];
    if(object.type==t_table){
        return to_string(table_stringify(E, object.tp));
    } else
    {
        return to_string(stringify(E, object));
    }
}

void initialize_prototype_chain(Executor* E){
    ObjectSystem* object_system=OBJECT_SYSTEM(E);
    Object any_type=object_system->types_objects[t_any];

    // override symbol is saved as enclosing scope of the function
    // it is checked later to not execute this functions infinitely
    #define REGISTER(override, arguments_count, variadic, native_function) \
        set_function(E, any_type, OVERRIDE(E, override), arguments_count, variadic, native_function); \
        table_get(E, any_type.tp, OVERRIDE(E, override)).fp->enclosing_scope=OVERRIDE(E, override); \
        reference(&OVERRIDE(E, override));

    REGISTER(call, 2, false, any_call)
    REGISTER(operator, 3, false, any_operator)
    REGISTER(cast, 2, false, any_cast);
    REGISTER(compare, 2, false, any_compare);
    REGISTER(hash, 1, false, any_hash);
    REGISTER(iterator, 1, false, any_iterator);
    REGISTER(stringify, 1, false, any_stringify);

    #undef REGISTER
}