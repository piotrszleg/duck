#ifndef RECORD_CHANGES_H
#define RECORD_CHANGES_H

#include "bytecode_manipulation.h"

typedef enum {
    ct_add,
    ct_remove,
    ct_replace
} ChangeType;

extern const char* CHANGE_TYPE_NAMES[];

typedef struct {
    uint line;
    ChangeType type;
} Change;

void initialize_recording(BytecodeManipulation* manipulation);
void finish_recording(BytecodeManipulation* manipulation);
// last element of highlighted_lines should be -1, highlighted_lines can also be NULL
void begin_recording_change(BytecodeManipulation* manipulation, char* name, 
                            int* highlighted_lines);
void move_changes_forward(BytecodeManipulation* manipulation, int starting_index);
void add_change(BytecodeManipulation* manipulation, ChangeType type, uint line);
void end_recording_change(BytecodeManipulation* manipulation);

#endif