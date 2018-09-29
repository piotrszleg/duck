#ifndef OBJECT_H
#define OBJECT_H
#include "map.h"
#include "error.h"
#include <stdlib.h>
#include <ctype.h>

typedef enum object_type object_type;
enum object_type{
    t_null,
    t_number,
    t_function,
    t_string,
    t_table
};

extern const char* OBJECT_TYPE_NAMES[];// array mapping enum object_type to their names as strings

typedef struct object object;
struct object{// object interface, all objects in this file implement it
    enum object_type type;
};

#define RUNTIME_OBJECT(t, body) \
    typedef struct t t; \
    struct t { \
        object_type type; \
        body \
    }; \
    t* new_ ## t(); \

RUNTIME_OBJECT(null,)

RUNTIME_OBJECT(number,
    float value;
)

RUNTIME_OBJECT(string,
    char* value;
)

typedef map_t(struct object*) object_map_t;

RUNTIME_OBJECT(table,
    object_map_t fields;
)

RUNTIME_OBJECT(function,
    object* (*pointer)(object* o, table*);
    int arguments_count;
    void* data;
);

char* stringify(object* o);

object* object_new(object_type);
void object_delete(object* o);

object* add(object* a, object* b);

object* call(object* o, table* arguments);

object* get(object* o, char*key);
void set(object* o, char*key, object* value);

#endif