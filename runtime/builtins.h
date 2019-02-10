#ifndef BUILTINS_H
#define BUILTINS_H

#include <string.h>
#include "../object_system/object.h"
#include "../object_system/object_operations.h"
#include "../error/error.h"
#include "../macros.h"
#include "../runtime/struct_descriptor.h"

void register_builtins(object scope);
void inherit_scope(object scope, object base);
char* fgets_no_newline(char *buffer, size_t buflen, FILE* fp);

#endif