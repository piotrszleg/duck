#ifndef OBJECT_OPERATIONS_H
#define OBJECT_OPERATIONS_H

#include "object.h"
#include "table.h"
#include "../utility.h"
#include <stdbool.h>

#define STRINGIFY_BUFFER_SIZE 32

extern object patching_table;

bool is_falsy(object o);

int compare(object a, object b);
object operator(executor* Ex, object a, object b, const char* op);

object cast(executor* Ex, object o, object_type type);

object call(executor* Ex, object o, object* arguments, int arguments_count);

object get(executor* Ex, object o, object key);
object set(executor* Ex, object o, object key, object value);

void get_execution_info(executor* Ex, char* buffer, int buffer_count);
object multiple_causes(executor* Ex, object* causes, int causes_count);
object new_error(executor* Ex, char* type, object cause, char* message, char* location);

char* suprintf (const char * format, ...);
char* stringify_object(executor* Ex, object o);
char* stringify(executor* Ex, object o);
object get_iterator(executor* Ex, object o);

// iterated and receiver should be of object type, executes body with receiver becoming subsequential elements of iterated
#define FOREACH(iterated, receiver, body) \
    { \
        object iterator=get_iterator(Ex, iterated); \
        reference(&iterator); \
        \
        receiver=call(Ex, iterator, NULL, 0); \
        while(true) { \
            reference(&receiver); \
            {body} \
            dereference(Ex, &receiver); \
            receiver=call(Ex, iterator, NULL, 0); \
            if(!is_falsy(get(Ex, receiver, to_string("finished")))){ \
                dereference(Ex, &iterator); \
                break; \
            } \
        } \
    }

#include "error_object.h"
#include "binding_object.h"
#include "pipe_object.h"

#endif