#ifndef RECORD_CHANGES_H
#define RECORD_CHANGES_H

#include "bytecode_manipulation.h"

void initialize_recording(BytecodeManipulation* manipulation);
void finish_recording(BytecodeManipulation* manipulation);
// last element of highlighted_lines should be -1, highlighted_lines can also be NULL
void begin_recording_change(BytecodeManipulation* manipulation, char* name, int* highlighted_lines);
void end_recording_change(BytecodeManipulation* manipulation);

#endif