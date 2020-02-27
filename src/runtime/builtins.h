#ifndef BUILTINS_H
#define BUILTINS_H

#include <string.h>
#include <time.h>
#include "../object_system/object.h"
#include "../object_system/object_operations.h"
#include "../error/error.h"
#include "../utility.h"
#include "../bytecode/execute_bytecode.h"
#include "../execution/execution.h"
#include "../callables/coroutine.h"
#include "struct_descriptor.h"
#include "import_dll.h"
#include "../transformers/expression_object.h"

Object builtins_table(Executor* E);
void inherit_scope(Executor* E, Table* scope, Object base);
void inherit_global_scope(Executor* E, Table* scope);
void inherit_current_scope(Executor* E, Table* scope);
bool scope_inherits(Executor* E, Object scope, Object presumed_base);

#endif