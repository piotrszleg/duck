#ifndef OBJECT_H
#define OBJECT_H
#include <stdlib.h>
#include <math.h>
#include "map.h"
#include "../error/error.h"
#include <ctype.h>
#include "vector.h"

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
    int ref_count;
};

#define RUNTIME_OBJECT(t, body) \
    typedef struct t t; \
    struct t { \
        object_type type; \
        int ref_count; \
        body \
    }; \
    t* new_ ## t(); \

#define RUNTIME_OBJECT_NEW(t, body) \
    t* new_ ## t(){  \
        t* instance=malloc(sizeof(t)); \
        CHECK_ALLOCATION(instance); \
		instance->type=t_ ## t; \
        if(ALLOC_LOG) printf("new " #t "\n");\
        instance->ref_count=0; \
        body \
        return instance; \
    }

#define CHECK_OBJECT(checked) \
    if(checked==NULL) { ERROR(INCORRECT_OBJECT_POINTER, "Object pointer \"" #checked "\" passed to function %s is null", __FUNCTION__); } \
    if(checked->type<t_null||checked->type>t_table) { ERROR(INCORRECT_OBJECT_POINTER, "Object \"" #checked "\" passed to function %s has incorrect type value %i", __FUNCTION__, checked->type); }

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
    object* (*pointer)(object* o, table* scope);
    int is_native;
    table* enclosing_scope;
    vector argument_names;
    void* data;
);

char* stringify(object* o);

void delete_unreferenced(object* checked);

void object_delete(object* o);

int is_falsy(object* o);

object* operator(object* a, object* b, char* op);

object* call(object* o, table* arguments);

object* get(object* o, char*key);
void set(object* o, char*key, object* value);

#endif