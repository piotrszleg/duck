#ifndef OBJECT_OPERATIONS_H
#define OBJECT_OPERATIONS_H

#include "object.h"
#include "..\macros.h"
#include <stdbool.h>

bool is_falsy(object o);

int compare(object a, object b);
object operator(object a, object b, const char* op);

object cast(object o, object_type type);

object call(object o, object* arguments, int arguments_count);

// object_system doesn't know how to execute code other than native, 
// so this function should be implemented in higher level module
object call_function(function* f, object* arguments, int arguments_count);

object get_table(table* t, const char* key);
object get(object o, const char* key);
void set_table(table* t, const char* key, object value);
void set(object o, const char* key, object value);

void get_execution_info(char* buffer, int buffer_count);
object multiple_causes(object* causes, int causes_count);
object new_error(char* type, object cause, char* message, char* location);

char* stringify_object(object o);
char* stringify(object o);

#define ERROR_BUFFER_SIZE 100
#define RETURN_ERROR(type, cause, message, ...) \
    { char* location=malloc(ERROR_BUFFER_SIZE*sizeof(char)); \
    char e_info[ERROR_BUFFER_SIZE]; \
    get_execution_info(e_info, ERROR_BUFFER_SIZE); \
    sprintf(location, "at:\t(%s)\nsrc:\t(%s:%d)", e_info, __FILE__, __LINE__); \
    char* message_formatted=malloc(ERROR_BUFFER_SIZE *sizeof(char)); \
    snprintf(message_formatted, ERROR_BUFFER_SIZE, message, ##__VA_ARGS__); \
    return new_error(type, cause, message_formatted, location); }

#endif