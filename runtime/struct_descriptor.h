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
    n_struct
} native_type;

object new_struct_descriptor(void* position, object sclass);
object to_field(int offset, native_type type);

#endif