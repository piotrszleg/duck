#ifndef BUILTINS_H
#define BUILTINS_H

#include <string.h>
#include <time.h>
#include "../object_system/object.h"
#include "../object_system/object_operations.h"
#include "../error/error.h"
#include "../utility.h"
#include "../execute_bytecode.h"
#include "../execution.h"
#include "../coroutine.h"
#include "struct_descriptor.h"
#include "import_dll.h"

Object builtins_table(Executor* E);
void inherit_scope(Executor* E, Object scope, Object base);
bool scope_inherits(Executor* E, Object scope, Object presumed_base);

#endif