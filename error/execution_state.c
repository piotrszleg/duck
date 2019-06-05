#include "execution_state.h"

void get_execution_info(executor* Ex, char* buffer, int buffer_count){
    const char* file="unknown";
    if(Ex->file!=NULL){
        file=Ex->file;
    }
    snprintf(buffer, buffer_count, "%s:%i:%i", file, Ex->line, Ex->column);
}