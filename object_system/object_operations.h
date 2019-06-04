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
object operator(object a, object b, const char* op);

object cast(object o, object_type type);

object call(object o, object* arguments, int arguments_count);

object get(object o, object key);
object set(object o, object key, object value);

void get_execution_info(char* buffer, int buffer_count);
object multiple_causes(object* causes, int causes_count);
object new_error(char* type, object cause, char* message, char* location);

char* suprintf (const char * format, ...);
char* stringify_object(object o);
char* stringify(object o);
object get_iterator(object o);

// iterated and receiver should be of object type, executes body with receiver becoming subsequential elements of iterated
#define FOREACH(iterated, receiver, body) \
    { \
        object iterator=get_iterator(iterated); \
        reference(&iterator); \
        \
        receiver=call(iterator, NULL, 0); \
        while(true) { \
            reference(&receiver); \
            {body} \
            dereference(&receiver); \
            receiver=call(iterator, NULL, 0); \
            if(!is_falsy(get(receiver, to_string("finished")))){ \
                dereference(&iterator); \
                break; \
            } \
        } \
    }

#include "error_object.h"
#include "binding_object.h"
#include "pipe_object.h"

#endif