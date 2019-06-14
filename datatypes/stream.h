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
int stream_push(stream* s, const void* data_pointer, size_t size);
int stream_push_string(stream* s, const char* string);
void stream_truncate(stream* s);
void* stream_get_data(stream* s);

#endif