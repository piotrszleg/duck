#ifndef OBJECT_OPERATIONS_H
#define OBJECT_OPERATIONS_H

#include "object.h"
#include "..\macros.h"
#include <stdbool.h>

#define STRINGIFY_BUFFER_SIZE 200

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

#include "error_object.h"
#include "pipe_object.h"

#endif