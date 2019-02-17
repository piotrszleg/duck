#ifndef BUILTINS_H
#define BUILTINS_H

#include <string.h>
#include "../object_system/object.h"
#include "../object_system/object_operations.h"
#include "../error/error.h"
#include "../macros.h"
#include "../runtime/struct_descriptor.h"

#define REQUIRE(predicate, cause) if(!(predicate)) { RETURN_ERROR("WRONG_ARGUMENT", cause, "Requirement of function %s wasn't satisified: %s", __FUNCTION__, #predicate); }
#define REQUIRE_TYPE(o, t) if(o.type!=t) { \
    RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Wrong type of argument \"%s\" passed to function %s, it should be %s.", #o, __FUNCTION__, OBJECT_TYPE_NAMES[t]); }


void register_builtins(object scope);
void inherit_scope(object scope, object base);
void clean_scope_table(object scope);
char* fgets_no_newline(char *buffer, size_t buflen, FILE* fp);

#endif