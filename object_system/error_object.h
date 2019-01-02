#ifndef ERROR_OBJECT_H
#define ERROR_OBJECT_H

#include "object.h"
#include "object_operations.h"

#define ERROR_BUFFER_SIZE 100
#define RETURN_ERROR(type, cause, message, ...) \
    { char* location=malloc(ERROR_BUFFER_SIZE*sizeof(char)); \
    char e_info[ERROR_BUFFER_SIZE]; \
    get_execution_info(e_info, ERROR_BUFFER_SIZE); \
    sprintf(location, "at:\t(%s)\nsrc:\t(%s:%d)", e_info, __FILE__, __LINE__); \
    char* message_formatted=malloc(ERROR_BUFFER_SIZE *sizeof(char)); \
    snprintf(message_formatted, ERROR_BUFFER_SIZE, message, ##__VA_ARGS__); \
    return new_error(type, cause, message_formatted, location); }

object multiple_causes(object* causes, int causes_count);
object new_error(char* type, object cause, char* message, char* location);

#endif