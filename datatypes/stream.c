#include "stream.h"

void init_stream(stream* s, size_t size){
    s->data=malloc(size);
    CHECK_ALLOCATION(s->data);
    s->position=0;
    s->size=size;
}

int stream_push(stream* s, const void* data_pointer, size_t size){
    while(size>s->size-s->position){
        s->data=realloc(s->data, s->size*2);
        CHECK_ALLOCATION(s->data);
        s->size*=2;
    }
    int push_position=s->position;
    // void pointers can't be incremented normally so they need to be casted
    char* destination=(char*)s->data;
    destination+=push_position;

    memcpy(destination, data_pointer, size);
    s->position+=size;
    return push_position;
}

void stream_truncate(stream* s){
    if(s->position>0){
        s->data=realloc(s->data, s->position);
        CHECK_ALLOCATION(s->data);
    } else {
        free(s->data);
        s->data=NULL;
    }
}

void* stream_get_data(stream* s){
    return s->data;
}