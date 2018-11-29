#ifndef MACROS_H
#define MACROS_H

#include "error/error.h"

// creates string variable str, executes body and frees the string afterwards
#define USING_STRING(string_expression, body) { char* str=string_expression; body; free(str); }

#define CHECK_ALLOCATION(value) \
    if(value==NULL) { \
        ERROR(MEMORY_ALLOCATION_FAILURE, "Memory allocation failure in function %s", __FUNCTION__); \
    }

#endif