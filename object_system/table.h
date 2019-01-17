#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include "object.h"

#define INITIAL_MAP_SIZE 16
#define INITIAL_ARRAY_SIZE 16

typedef struct map_element map_element;
struct map_element {
    int hash;
    object key;
    object value;
    map_element* next;
};

struct table {
    int ref_count;
    object* array;
    int array_size;
    map_element** map;
    int map_size;
};

object get_table(table* t, object key);
void set_table(table* t, object key, object value);
void table_component_init(table* t);

#endif