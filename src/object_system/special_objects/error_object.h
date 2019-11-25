#ifndef ERROR_OBJECT_H
#define ERROR_OBJECT_H

#include <stdarg.h>
#include "../object.h"
#include "../object_operations.h"

#define ERROR_BUFFER_SIZE 128
#define NEW_ERROR(result, type, cause, message, ...) \
    { char* location=malloc(ERROR_BUFFER_SIZE*sizeof(char)); \
    char e_info[ERROR_BUFFER_SIZE]; \
    get_execution_info(E, e_info, ERROR_BUFFER_SIZE); \
    snprintf(location, ERROR_BUFFER_SIZE, "at:\t(%s)\nsrc:\t(%s:%d)", e_info, __FILE__, __LINE__); \
    char* message_formatted=malloc(ERROR_BUFFER_SIZE *sizeof(char)); \
    snprintf(message_formatted, ERROR_BUFFER_SIZE, message, ##__VA_ARGS__); \
    result=new_error(E, type, cause, message_formatted, location); \
    free(location); \
    free(message_formatted); \
    }
#define RETURN_ERROR(type, cause, message, ...) \
    { Object err; \
    NEW_ERROR(err, type, cause, message, ##__VA_ARGS__) \
    return err; }

#define MULTIPLE_CAUSES(...) \
    multiple_causes(E, OBJECTS_ARRAY(__VA_ARGS__), sizeof(OBJECTS_ARRAY(__VA_ARGS__))/sizeof(Object))

Object multiple_causes(Executor* E, Object* causes, int causes_count);
Object new_error(Executor* E, char* type, Object cause, char* message, char* location);
bool is_error(Executor* E, Object o);
bool is_unhandled_error(Executor* E, Object o);
void handle_if_error(Executor* E, Object o);
void error_handle(Executor* E, Object o);

#endif