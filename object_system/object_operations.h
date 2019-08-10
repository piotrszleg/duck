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

// it can be safely casted to ObjectType if the value is greater than zero
typedef enum {
    tu_unknown = -1,
    #define X(type) tu_##type=t_##type,
    OBJECT_TYPES
    #undef X
} ObjectTypeOrUnknown;
ObjectTypeOrUnknown operator_predict_result(ObjectTypeOrUnknown a, ObjectTypeOrUnknown b, const char* op);
Object operator(Executor* E, Object a, Object b, const char* op);

#define OBJECTS_ARRAY(...) ((Object[]){__VA_ARGS__})
#define OPERATOR_OVERRIDE_FAILURE \
    RETURN_ERROR("OPERATOR_ERROR", multiple_causes(E, OBJECTS_ARRAY(arguments[0], arguments[1]), 2), \
    "Can't perform operotion '%s' on objects of type <%s> and <%s>", arguments[2].text, get_type_name(arguments[0].type), get_type_name(arguments[1].type));

Object cast(Executor* E, Object o, ObjectType type);
Object call(Executor* E, Object o, Object* arguments, int arguments_count);
Object get(Executor* E, Object o, Object key);
Object set(Executor* E, Object o, Object key, Object value);

void get_execution_info(Executor* E, char* buffer, int buffer_count);
Object multiple_causes(Executor* E, Object* causes, int causes_count);
Object new_error(Executor* E, char* type, Object cause, char* message, char* location);

bool is_serializable(Object o);
char* serialize(Executor* E, Object o);
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