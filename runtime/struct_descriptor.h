#ifndef STRUCT_DESCRIPTOR_H
#define STRUCT_DESCRIPTOR_H

#include <string.h>
#include "../object_system/object.h"
#include "../object_system/object_operations.h"
#include "../error/error.h"
#include "../utility.h"

#define INT_TYPES \
    X(char) \
    X(int) \
    X(int8_t) \
    X(int16_t) \
    X(int32_t) \
    X(int64_t) \
    X(uint8_t) \
    X(uint16_t) \
    X(uint32_t) \
    X(uint64_t)

typedef enum {
    #define X(t) n_##t,
    INT_TYPES
    #undef X
    n_float,
    n_double,
    n_long_double,
    n_string,
    n_struct,
    n_pointer
} NativeType;

extern char* NATIVE_TYPES_NAMES[];

Object new_struct_descriptor(Executor* E, void* position, Object fields);
Object to_field(Executor* E, int offset, NativeType type);
Object to_struct_field(Executor* E, int offset, Object fields);
Object to_struct_pointer_field(Executor* E, int offset, Object fields);
void* struct_descriptor_get_pointer(Executor* E, Table* sd);
bool is_struct_descriptor(Executor* E, Object o);

#define OFFSET(structure, field) (long long)&(structure).field-(long long)&(structure)

#endif