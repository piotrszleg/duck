#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include <stdint.h>
#include "object.h"
#include "../datatypes/stream.h"
#include "../utility.h"
#include "error_object.h"

#define INITIAL_MAP_SIZE 16
#define INITIAL_ARRAY_SIZE 16

#define REQUIRE_TYPE(o, t) if(o.type!=t) { \
    RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Wrong type of argument \"%s\" passed to function %s, it should be %s.", #o, __FUNCTION__, OBJECT_TYPE_NAMES[t]); }

typedef struct MapElement MapElement;
struct MapElement {
    Object key;
    Object value;
    MapElement* next;
};

struct Table {
    // gc_Object fields
    gc_Object gco;

    Object* array;
    unsigned array_size;
    MapElement** map;
    unsigned map_size;
};

typedef struct {
    Table* iterated;
    bool inside_array;
    int index;
    MapElement* element;
} TableIterator;

typedef struct {
    bool inside_array;
    bool finished;
    Object key;
    Object value;
} IterationResult;

Object get_table(Table* t, Object key);
void set_table(Executor* E, Table* t, Object key, Object value);
void free_table(Table* t);
void dereference_children_table(Executor* E, Table* t);
char* stringify_table(Executor* E, Table* t);
void table_component_init(Table* t);
Object get_table_iterator(Executor* E, Object* arguments, int arguments_count);
TableIterator start_iteration(Table* iterated);
IterationResult table_next(TableIterator* it);

#endif