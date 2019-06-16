#include "stream.h"

void stream_init(stream* s, size_t size){
    s->data=malloc(size);
    CHECK_ALLOCATION(s->data);
    s->position=0;
    s->size=size;
}

void stream_fit(stream* s, int minimal_size){
    s->size=nearest_power_of_two(minimal_size);
    s->data=realloc(s->data, s->size*2);
    CHECK_ALLOCATION(s->data);
}

int stream_push_string(stream* s, const char* string){
    int push_position=s->position;
    for(int i=0; string[i]!='\0'; i++){
        if(s->position>=s->size){
            stream_fit(s, s->position+1);
        }
        ((char*)s->data)[s->position]=string[i];
        s->position++;
    }
    return push_position;
}

int stream_push(stream* s, const void* data_pointer, size_t size){
    if(s->position+size>s->size){
        stream_fit(s, s->position+size);
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

#define stream_printf(s, format, ...) \
    int available_space=(s)->size-s->position; \
    int needed_characters=snprintf(((char*)(s)->data)+(s)->position, available_space, (format), __VA_ARGS__); \
    if(needed_characters<0) { \
        THROW_ERROR(PRINTF_ERROR, "Snprintf function failed."); \
    } \
    if(needed_characters>=available_space) { \
        stream_fit((s)->position+needed_characters+1); \
        available_space=(s)->size-(s)->position; \
        int second_write_result=snprintf(((char*)(s)->data)+s->position, available_space, (format), __VA_ARGS__); \
        if(second_write_result<0 || second_write_result>=available_space) { \
            THROW_ERROR(PRINTF_ERROR, "Snprintf function failed."); \
        } \
    }

void* stream_get_data(stream* s){
    return s->data;
}