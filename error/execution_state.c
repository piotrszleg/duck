#include "execution_state.h"

execution_state exec_state;

void get_execution_info(char* buffer, int buffer_count){
    const char* file="unknown";
    if(exec_state.file!=NULL){
        file=exec_state.file;
    }
    snprintf(buffer, buffer_count, "%s:%i:%i", file, exec_state.line, exec_state.column);
}