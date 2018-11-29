#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <setjmp.h>

enum error_type{
    MEMORY_ALLOCATION_FAILURE,
    WRONG_ARGUMENT_TYPE,
    TYPE_CONVERSION_FAILURE,
    INCORRECT_OBJECT_POINTER,
    BUFFER_TOO_SMALL,
    NOT_IMPLEMENTED
};

extern jmp_buf error_buf;
extern char err_type;
extern char err_message[1024];

// save the previous error handler to allow nesting TRY_CATCH expressions
// execute try_block, if ERROR macro is called in it call catch_block
// if catch_block doesn't exit program execution it should return execution to the safe state
#define TRY_CATCH(try_block, catch_block) \
    jmp_buf previous_error_buf; \
    memcpy(previous_error_buf, error_buf, sizeof error_buf); \
    if (!setjmp(error_buf)){ try_block } \
    else { catch_block } \
    if(previous_error_buf!=NULL){ \
        memcpy(error_buf, previous_error_buf, sizeof error_buf); \
    }

#define ERROR(type, message, ...) \
    sprintf(err_message, "ERROR: %s, line %d\n" message, __FILE__, __LINE__, ##__VA_ARGS__); \
    err_type=type; \
    if(error_buf!=NULL){ longjmp(error_buf,1); }

#define CHECK_ALLOCATION(value) \
    if(value==NULL) { \
        ERROR(MEMORY_ALLOCATION_FAILURE, "Memory allocation failure in function %s", __FUNCTION__); \
    }


#endif