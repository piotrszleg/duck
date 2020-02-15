#ifndef BYTECODE_TO_LIGHTNING_H
#define BYTECODE_TO_LIGHTNING_H

#define HAS_LIGHTNING

#include "../bytecode/bytecode_program.h"
#include "../utility.h"

#ifdef HAS_LIGHTNING
#include "lightning.h"
#endif

void compile_bytecode_program(Executor* E, BytecodeProgram* program);

#endif