#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include "object.h"
#include "../datatypes/stream.h"
#include "../macros.h"

#define INITIAL_MAP_SIZE 16
#define INITIAL_ARRAY_SIZE 16

typedef struct map_element map_element;
struct map_element {
    object key;
    object value;
    map_element* next;
};

struct table {
    int ref_count;
    object* array;
    unsigned array_size;
    map_element** map;
    unsigned map_size;
};

typedef struct table_iterator table_iterator;
struct table_iterator{
    table* iterated;
    bool inside_array;
    int index;
    map_element* element;
};

typedef struct iteration_result iteration_result;
struct iteration_result{
    bool inside_array;
    bool finished;
    object key;
    object value;
};

object get_table(table* t, object key);
void set_table(table* t, object key, object value);
char* stringify_table(table* t);
void table_component_init(table* t);

#endif