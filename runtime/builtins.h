#ifndef BUILTINS_H
#define BUILTINS_H

#include <string.h>
#include "../object_system/object.h"
#include "../object_system/object_operations.h"
#include "../error/error.h"
#include "../utility.h"
#include "struct_descriptor.h"
#include "import_dll.h"

#define REQUIRE(predicate, cause) if(!(predicate)) { RETURN_ERROR("WRONG_ARGUMENT", cause, "Requirement of function %s wasn't satisified: %s", __FUNCTION__, #predicate); }
#define REQUIRE_TYPE(o, t) if(o.type!=t) { \
    RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Wrong type of argument \"%s\" passed to function %s, it should be %s.", #o, __FUNCTION__, OBJECT_TYPE_NAMES[t]); }


void register_builtins(executor* Ex, object scope);
void inherit_scope(executor* Ex, object scope, object base);

#endif