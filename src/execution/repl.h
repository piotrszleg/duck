#ifndef REPL_H
#define REPL_H

#include <stdio.h>
#include "../parser/parser.h"
#include "execution.h"
#include "../runtime/builtins.h"
#include "../utility.h"

void repl(Executor* E);

#endif