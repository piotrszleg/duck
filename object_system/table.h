#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include <stdint.h>
#include "object.h"
#include "object_operations.h"
#include "../datatypes/stream.h"
#include "../utility.h"
#include "error_object.h"

#define INITIAL_MAP_SIZE 32
#define INITIAL_ARRAY_SIZE 16

typedef struct MapElement MapElement;
struct MapElement {
    Object key;
    Object value;
    MapElement* next;
};

struct Table {
    gc_Object gco;

    Object* array;
    unsigned elements_count;
    unsigned array_size;
    MapElement** map;
    unsigned map_size;
    bool protected;
    bool special_fields_disabled;
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

bool is_valid_name(char* s);

unsigned table_hash(Executor* E, Table* t, Object* error);
Object table_get(Executor* E, Table* t, Object key);
void table_set(Executor* E, Table* t, Object key, Object value);
void table_protect(Table* t);
bool table_is_protected(Table* t);
void table_disable_special_fields(Table* t);
bool table_has_special_fields(Table* t);
void table_free(Table* t);
void table_foreach_children(Executor* E, Table* t, gc_PointerForeachChildrenCallback callback);
int table_compare(Executor* E, Table* a, Table* b, Object* error);
char* table_stringify(Executor* E, Table* t);
Object table_copy(Executor* E, Table* t);
void table_component_init(Table* t);
Object table_get_iterator_object(Executor* E, Object scope, Object* arguments, int arguments_count);
TableIterator table_get_iterator(Table* iterated);
IterationResult table_iterator_next(TableIterator* it);

#endif