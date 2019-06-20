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
    unsigned elements_count;
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

Object table_get(Table* t, Object key);
void table_set(Executor* E, Table* t, Object key, Object value);
void free_table(Table* t);
void table_dereference_children(Executor* E, Table* t);
char* stringify_table(Executor* E, Table* t);
void table_component_init(Table* t);
Object table_get_iterator_object(Executor* E, Object* arguments, int arguments_count);
TableIterator table_get_iterator(Table* iterated);
IterationResult table_iterator_next(TableIterator* it);

bool is_valid_name(char* s);

#endif