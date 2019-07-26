#ifndef OBJECT_OPERATIONS_H
#define OBJECT_OPERATIONS_H

#include "object.h"
#include "Table.h"
#include "../utility.h"
#include <stdbool.h>

#define STRINGIFY_BUFFER_SIZE 32

bool is_falsy(Object o);
bool is_truthy(Object o);

int compare(Executor* E, Object a, Object b);
int compare_and_get_error(Executor* E, Object a, Object b, Object* error);
bool is(Executor* E, Object a, Object b);
unsigned hash(Executor* E, Object o, Object* error);
Object operator(Executor* E, Object a, Object b, const char* op);

Object cast(Executor* E, Object o, ObjectType type);

Object call(Executor* E, Object o, Object* arguments, int arguments_count);
Object message_object(Executor* E, Object messaged, const char* message_identifier, Object* arguments, int arguments_count);

Object get(Executor* E, Object o, Object key);
Object set(Executor* E, Object o, Object key, Object value);

void get_execution_info(Executor* E, char* buffer, int buffer_count);
Object multiple_causes(Executor* E, Object* causes, int causes_count);
Object new_error(Executor* E, char* type, Object cause, char* message, char* location);

Object copy(Executor* E, Object o);
char* suprintf (const char * format, ...);
char* stringify_object(Executor* E, Object o);
char* stringify(Executor* E, Object o);
Object get_iterator(Executor* E, Object o);

#include "error_object.h"
#include "binding_object.h"
#include "pipe_object.h"

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