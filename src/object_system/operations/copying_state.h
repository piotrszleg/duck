#ifndef COPYING_STATE_H
#define COPYING_STATE_H

typedef struct {
    Table* copies;
    int identifiers_counter;
    Table* identifier_to_copied;
} CopyingState;

#endif