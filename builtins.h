#ifndef BUILTINS_H
#define BUILTINS_H

#include <string.h>
#include "object_system/object.h"
#include "error/error.h"
#include "macros.h"

#define BUILTIN_ARGUMENTS_CHECK(arguments_count) \
    if(vector_total(&arguments)<arguments_count){ \
         ERROR(NOT_ENOUGH_ARGUMENTS, "Not enough arguments, expected %i arguments.", arguments_count); \
    }

void register_builtins(table* scope);
void setup_scope(object* scope, object* base);

#endif