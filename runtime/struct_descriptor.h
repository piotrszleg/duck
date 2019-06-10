#ifndef STRUCT_DESCRIPTOR_H
#define STRUCT_DESCRIPTOR_H

#include <string.h>
#include "../object_system/object.h"
#include "../object_system/object_operations.h"
#include "../error/error.h"
#include "../utility.h"

// TODO: implement correct behaviour for struct pointers
typedef enum {
    n_string,
    n_int,
    n_float,
    n_struct,
    n_pointer
} NativeType;

Object new_struct_descriptor(Executor* E, void* position, Object sclass);
Object to_field(Executor* E, int offset, NativeType type);
Object to_struct_field(Executor* E, int offset, Object class);
Object to_struct_pointer_field(Executor* E, int offset, Object class);
void* struct_descriptor_get_pointer(Executor* E, Table* sd);
bool is_struct_descriptor(Executor* E, Object o);

#define OFFSET(structure, field) (int)&(structure).field-(int)&(structure)

#endif