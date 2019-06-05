#ifndef OBJECT_OPERATIONS_H
#define OBJECT_OPERATIONS_H

#include "object.h"
#include "Table.h"
#include "../utility.h"
#include <stdbool.h>

#define STRINGIFY_BUFFER_SIZE 32

extern Object patching_table;

bool is_falsy(Object o);

int compare(Object a, Object b);
Object operator(Executor* E, Object a, Object b, const char* op);

Object cast(Executor* E, Object o, ObjectType type);

Object call(Executor* E, Object o, Object* arguments, int arguments_count);

Object get(Executor* E, Object o, Object key);
Object set(Executor* E, Object o, Object key, Object value);

void get_execution_info(Executor* E, char* buffer, int buffer_count);
Object multiple_causes(Executor* E, Object* causes, int causes_count);
Object new_error(Executor* E, char* type, Object cause, char* message, char* location);

char* suprintf (const char * format, ...);
char* stringify_object(Executor* E, Object o);
char* stringify(Executor* E, Object o);
Object get_iterator(Executor* E, Object o);

// iterated and receiver should be of object type, executes body with receiver becoming subsequential elements of iterated
#define FOREACH(iterated, receiver, body) \
    { \
        Object iterator=get_iterator(E, iterated); \
        reference(&iterator); \
        \
        receiver=call(E, iterator, NULL, 0); \
        while(true) { \
            reference(&receiver); \
            {body} \
            dereference(E, &receiver); \
            receiver=call(E, iterator, NULL, 0); \
            if(!is_falsy(get(E, receiver, to_string("finished")))){ \
                dereference(E, &iterator); \
                break; \
            } \
        } \
    }

#include "error_object.h"
#include "binding_object.h"
#include "pipe_object.h"

#endif