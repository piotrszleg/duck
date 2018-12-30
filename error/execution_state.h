#ifndef EXECUTION_STATE
#define EXECUTION_STATE

#include <stdio.h>

typedef struct execution_state execution_state;
struct execution_state {
    unsigned line;
    unsigned column;
    unsigned* traceback;
    const char* file;
};

extern execution_state exec_state;

#endif