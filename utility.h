#ifndef UTILITY_H
#define UTILITY_H

#include <string.h>
#include "error/error.h"

// creates string variable str, executes body and frees the string afterwards
#define USING_STRING(string_expression, body) { char* str=string_expression; body; free(str); }

#define CHECK_ALLOCATION(value) \
    if(value==NULL) { \
        THROW_ERROR(MEMORY_ALLOCATION_FAILURE, "Memory allocation failure in function %s", __FUNCTION__); \
    }

char* fgets_no_newline(char *buffer, size_t buflen, FILE* fp);

#endif