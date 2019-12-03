#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "../containers/vector.h"

typedef struct {
    char* file;
    int line;
} Breakpoint;

typedef struct {
    vector breakpoints;
    bool running;
} Debugger;

#include "executor.h"

void debugger_init(Debugger* debugger);
void debugger_deinit(Debugger* debugger);
void debugger_update(Executor* E, Debugger* debugger);

#endif