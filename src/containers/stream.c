#include "stream.h"

void stream_init(stream* s, size_t size){
    s->data=malloc(size);
    CHECK_ALLOCATION(s->data);
    s->position=0;
    s->size=size;
}

void stream_deinit(stream* s){
    free(s->data);
}

void stream_fit(stream* s, int minimal_size){
    if(s->size<minimal_size){
        s->size=nearest_power_of_two(minimal_size);
        s->data=realloc(s->data, s->size);
        CHECK_ALLOCATION(s->data);
    }
}

int stream_push_string(stream* s, const char* string){
    int push_position=s->position;
    for(int i=0; string[i]!='\0'; i++){
        stream_fit(s, s->position+1);
        ((char*)s->data)[s->position]=string[i];
        s->position++;
    }
    return push_position;
}

size_t stream_size(stream* s){
    return s->position;
}

int stream_push_string_indented(stream* s, const char* string){
    int push_position=s->position;
    
    for(int i=0; string[i]!='\0'; i++){
        stream_fit(s, s->position+1);
        ((char*)s->data)[s->position]=string[i];
        s->position++;
        if(string[i]=='\n'){
            stream_fit(s, s->position+1);
            ((char*)s->data)[s->position]='\t';
            s->position++;
        }
    }
    return push_position;
}

int stream_push(stream* s, const void* data_pointer, size_t size){
    stream_fit(s, s->position+size);
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

void stream_clear(stream* s){
    s->position=0;
}