#ifndef BYTECODE_TO_LIGHTNING_H
#define BYTECODE_TO_LIGHTNING_H

#define HAS_MYJIT

#include "bytecode_program.h"
#ifdef HAS_MYJIT
#include "myjit/jitlib.h"
#endif

void compile_bytecode_program(Executor* E, BytecodeProgram* program);

#endif