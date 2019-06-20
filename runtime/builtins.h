#ifndef BUILTINS_H
#define BUILTINS_H

#include <string.h>
#include "../object_system/object.h"
#include "../object_system/object_operations.h"
#include "../error/error.h"
#include "../utility.h"
#include "../execute_bytecode.h"
#include "../execution.h"
#include "struct_descriptor.h"
#include "import_dll.h"


void register_builtins(Executor* E, Object scope);
void inherit_scope(Executor* E, Object scope, Object base);

#endif