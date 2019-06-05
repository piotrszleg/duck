#include "execution_state.h"

void get_execution_info(Executor* E, char* buffer, int buffer_count){
    const char* file="unknown";
    if(E->file!=NULL){
        file=E->file;
    }
    snprintf(buffer, buffer_count, "%s:%i:%i", file, E->line, E->column);
}