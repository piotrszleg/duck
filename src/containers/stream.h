#ifndef STREAM_H
#define STREAM_H

#include <stdlib.h>
#include <string.h>
#include "../utility.h"

typedef struct stream stream;
struct stream {
    void* data;
    size_t position;// position inside of data in bytes
    size_t size;// size of the stream in bytes
};

void stream_init(stream* s, size_t size);
void stream_deinit(stream* s);
int stream_push(stream* s, const void* data_pointer, size_t size);
int stream_push_string(stream* s, const char* string);
int stream_push_string_indented(stream* s, const char* string);
void stream_truncate(stream* s);
void* stream_get_data(stream* s);
size_t stream_size(stream* s);
void stream_fit(stream* s, int minimal_size);

#define stream_push_const_string(s, string) \
    stream_push(s, string, sizeof(string)-1)

#define stream_printf(s, format, ...) \
    int available_space=(s)->size-(s)->position; \
    int needed_characters=snprintf(((char*)(s)->data)+(s)->position, available_space, (format), __VA_ARGS__); \
    if(needed_characters<0) { \
        CRITICAL_ERROR(PRINTF_ERROR, "Snprintf function failed."); \
    } \
    if(needed_characters>=available_space) { \
        stream_fit(s, (s)->position+needed_characters+1); \
        available_space=(s)->size-(s)->position; \
        int second_write_result=snprintf(((char*)(s)->data)+(s)->position, available_space, (format), __VA_ARGS__); \
        if(second_write_result<0 || second_write_result>=available_space) { \
            CRITICAL_ERROR(PRINTF_ERROR, "Snprintf function failed."); \
        } \
    } \
    (s)->position+=needed_characters;

#endif