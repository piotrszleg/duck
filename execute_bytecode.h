#ifndef EXECUTE_BYTECODE_H
#define EXECUTE_BYTECODE_H

#include "object_system/object.h"
#include "error/error.h"
#include "datatypes/stack.h"
#include "builtins.h"
#include "bytecode.h"
#include "macros.h"

object* execute_bytecode(instruction* code, void* constants, table* scope);

#endif