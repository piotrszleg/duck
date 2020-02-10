#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include <stdint.h>
#include "object.h"
#include "object_operations.h"
#include "../containers/stream.h"
#include "../utility.h"
#include "special_objects/error_object.h"
#include "operations/copying_state.h"

#define INITIAL_MAP_SIZE 32
#define INITIAL_ARRAY_SIZE 16

typedef struct MapElement MapElement;
struct MapElement {
    Object key;
    Object value;
    MapElement* next;
};

struct Table {
    HeapObject hp;

    Object* array;
    int elements_count;
    uint array_size;
    MapElement** map;
    uint map_size;
    bool protected;
};

typedef struct {
    Table* iterated;
    bool inside_array;
    uint index;
    MapElement* map_element;
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
Object table_get_with_hashing_error(Executor* E, Table* t, Object key, Object* error);
void table_set(Executor* E, Table* t, Object key, Object value);
void table_protect(Table* t);
bool table_is_protected(Table* t);
void table_free(Table* t);
void table_foreach_children(Executor* E, Table* t, ForeachChildrenCallback callback, void* data);
int table_compare(Executor* E, Table* a, Table* b, Object* error);
char* table_serialize(Executor* E, Table* t);
char* table_stringify(Executor* E, Table* t);
Object table_copy(Executor* E, Table* t, CopyingState* state);
void table_component_init(Table* t);
Object table_get_iterator_object(Executor* E, Object scope, Object* arguments, int arguments_count);
TableIterator table_get_iterator(Table* iterated);
IterationResult table_iterator_next(TableIterator* it);

#endif