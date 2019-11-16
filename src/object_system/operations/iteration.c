#include "iteration.h"

Object coroutine_iterator_next(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object iterator=arguments[0];
    Object coroutine=get(E, iterator, to_string("coroutine"));
    Object value=call(E, coroutine, NULL, 0);
    Object result;
    table_init(E, &result);
    set(E, result, to_string("value"), value);

    REQUIRE_TYPE(coroutine, t_coroutine);
    set(E, result, to_string("finished"), to_int(coroutine.co->state==co_finished));

    return result;
}

Object coroutine_iterator(Executor* E, Object coroutine){
    Object iterator;
    table_init(E, &iterator);
    set(E, iterator, to_string("coroutine"), coroutine);
    set_function(E, iterator, "next", 1, false, coroutine_iterator_next);
    return iterator;
}

Object string_iterator_next(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object iterator=arguments[0];
    Object iterated=get(E, iterator, to_string("iterated"));
    REQUIRE_TYPE(iterated, t_string)
    Object index=get(E, iterator, to_string("index"));
    REQUIRE_TYPE(index, t_int)
    Object length=get(E, iterator, to_string("length"));
    REQUIRE_TYPE(length, t_int)
    Object result;
    table_init(E, &result);
    if(index.int_value<length.int_value){
        set(E, result, to_string("key"), index);
        char character[2]={iterated.text[index.int_value], '\0'};
        set(E, result, to_string("value"), to_string(character));
        set(E, iterator, to_string("index"), to_int(index.int_value+1));
    } else {
        set(E, result, to_string("finished"), to_int(1));
    }
    return result;
}

Object string_iterator(Executor* E, Object str){
    Object iterator;
    table_init(E, &iterator);
    set(E, iterator, to_string("iterated"), str);
    set(E, iterator, to_string("index"), to_int(0));
    set(E, iterator, to_string("length"), to_int(strlen(str.text)));
    set_function(E, iterator, "next", 1, false, string_iterator_next);
    return iterator;
}

Object get_iterator(Executor* E, Object o){
    switch(o.type){
        case t_table: {
            Object iterator_override=get(E, o, OVERRIDE(E, iterator));
            if(iterator_override.type!=t_null){
                return call(E, iterator_override, &o, 1);
            } else {
                return table_get_iterator_object(E, null_const, &o, 1);
            }
        }
        case t_coroutine:
            return coroutine_iterator(E, o);
        case t_string:
            return string_iterator(E, o);
        default:;
    }
    PATCH(iterator, o.type, o)
    RETURN_ERROR("ITERATION_ERROR", o, "Can't get iterator of object of type %s.", get_type_name(o.type));
}