#include "object_history.h"

static const char* OBJECT_EVENTS_NAMES[]={
    #define X(t) #t,
    OBJECT_EVENTS
    #undef X
};

static const char* object_event_to_name(ObjectEvent event){
    return OBJECT_EVENTS_NAMES[event];
}

void object_history_init(ObjectHistory* history){
    vector_init(history, sizeof(ObjectHistoryEntry), 8);
}

void object_history_deinit(ObjectHistory* history){
    vector_deinit(history);
}

void object_history_add(ObjectHistory* history, ObjectEvent event, char* location){
    vector_push(history, &(ObjectHistoryEntry){event, location});
}
void object_history_print(ObjectHistory* history){
    for(int i=0; i<vector_count(history); i++){
        ObjectHistoryEntry* entry=vector_index(history, i);
        printf("%s %s\n", object_event_to_name(entry->event), entry->location);
    }
}