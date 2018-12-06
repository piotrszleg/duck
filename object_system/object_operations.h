#ifndef OBJECT_OPERATIONS_H
#define OBJECT_OPERATIONS_H

#include "object.h"
#include "..\macros.h"

int is_falsy(object o);

int compare(object a, object b);
object operator(object a, object b, char* op);

object cast(object o, object_type type);

object call(object o, vector arguments);

// object_system doesn't know how to execute code other than native, 
// so this function should be implemented in higher level module
object call_function(function_* f, vector arguments);

object get_table(table_* t, char* key);
object get(object o, char*key);
void set_table(table_* t, char*key, object value);
void set(object o, char*key, object value);

object new_error(char* type, object* cause, char* message);

char* stringify_object(object o);
char* stringify(object o);

#define RETURN_ERROR(type, cause, message, ...) \
    { char* error_message=malloc(STRINGIFY_BUFFER_SIZE*sizeof(char)); \
    sprintf(error_message, message, ##__VA_ARGS__); \
    return new_error(type, cause, error_message); }

#endif