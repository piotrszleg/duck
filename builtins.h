#ifndef BUILTINS_H
#define BUILTINS_H

#include "object_system/object.h"

void register_builtins(table* scope);
void setup_scope(object* scope, object* base);

#endif