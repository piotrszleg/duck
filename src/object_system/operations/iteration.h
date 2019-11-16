#ifndef ITERATION_H
#define ITERATION_H

#include "../object.h"
#include "patching.h"

Object get_iterator(Executor* E, Object o);

// iterated and receiver should be of type object, executes body with receiver becoming subsequential elements of iterated
#define FOREACH(iterated, receiver, body) \
    { \
        Object iterator=get_iterator(E, iterated); \
        if(is_error(E, iterator)){ \
            RETURN_ERROR("ITERATION_ERROR", iterator, "Object iterator is an error.") \
        } else if(iterator.type==t_null){ \
            RETURN_ERROR("ITERATION_ERROR", iterated, "Object doesn't have an iterator field.") \
        } \
        reference(&iterator); \
        Object next=get(E, iterator, to_string("next")); \
        if(is_error(E, next)){ \
            RETURN_ERROR("ITERATION_ERROR", next, "Object iterator's next is an error.") \
        } else if(next.type==t_null){ \
            RETURN_ERROR("ITERATION_ERROR", iterator, "Object's iterator doesn't have a next field.") \
        } \
        \
        while(true) { \
            receiver=call(E, next, &iterator, 1); \
            reference(&receiver); \
            if(is_error(E, receiver)){ \
                dereference(E, &receiver); \
                dereference(E, &iterator); \
                RETURN_ERROR("ITERATION_ERROR", receiver, "Iteration result is an error.") \
                return receiver; \
            } \
            if(is_truthy(get(E, receiver, to_string("finished")))){ \
                dereference(E, &receiver); \
                dereference(E, &iterator); \
                break; \
            } else { \
                body \
            } \
            dereference(E, &receiver); \
        } \
    }

#endif