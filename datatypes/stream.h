#ifndef STREAM_H
#define STREAM_H

#include <stdlib.h>
#include <string.h>
#include "../macros.h"

typedef struct stream stream;
struct stream {
    void* data;
    size_t position;// position inside of data in bytes
    size_t size;// size of the stream in bytes
};

void init_stream(stream* s, size_t size);
int stream_push(stream* s, void* data_pointer, size_t size);
void stream_truncate(stream* s);

#endif