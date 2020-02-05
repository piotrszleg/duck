#ifndef OBJECT_HISTORY_H
#define OBJECT_HISTORY_H

#include "../containers/vector.h"

#define OBJECT_EVENTS \
    X(reference) \
    X(dereference)

typedef enum {
    #define X(e) oe_##e,
    OBJECT_EVENTS
    #undef X
} ObjectEvent;

typedef struct {
    ObjectEvent event;
    char* location;
} ObjectHistoryEntry;

typedef vector/*<ObjectHistoryEntry>*/ ObjectHistory;

void object_history_add(ObjectHistory* history, ObjectEvent event, char* location);
void object_history_print(ObjectHistory*);
void object_history_init(ObjectHistory*);
void object_history_free(ObjectHistory*);

#endif