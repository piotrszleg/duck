#include "error.h"

jmp_buf error_buf;
char err_message[1024];
char err_type;

void critical_error_handler(ErrorType type){
    exit(type);
}