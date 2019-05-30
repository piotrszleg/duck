#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include <stdint.h>
#include "object.h"
#include "../datatypes/stream.h"
#include "../macros.h"
#include "error_object.h"

#define INITIAL_MAP_SIZE 16
#define INITIAL_ARRAY_SIZE 16

#define REQUIRE_TYPE(o, t) if(o.type!=t) { \
    RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Wrong type of argument \"%s\" passed to function %s, it should be %s.", #o, __FUNCTION__, OBJECT_TYPE_NAMES[t]); }

typedef struct map_element map_element;
struct map_element {
    object key;
    object value;
    map_element* next;
};

typedef struct table table;
struct table {
    // gc_object fields
    gc_object gco;

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
void free_table(table* t);
void dereference_children_table(table* t);
char* stringify_table(table* t);
void table_component_init(table* t);
object get_table_iterator(object* arguments, int arguments_count);
table_iterator start_iteration(table* iterated);
iteration_result table_next(table_iterator* it);

#endif