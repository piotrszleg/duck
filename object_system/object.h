#ifndef OBJECT_H
#define OBJECT_H
#include <stdlib.h>
#include <math.h>
#include "map.h"
#include "../error/error.h"
#include "..\macros.h"
#include <ctype.h>
#include "../datatypes/vector.h"

typedef enum object_type object_type;
enum object_type{
    t_null,
    t_number,
    t_function,
    t_string,
    t_table
};

extern const char* OBJECT_TYPE_NAMES[];// array mapping enum object_type to their names as strings

typedef struct table table;
typedef struct function function;

typedef struct object object;
struct object{
    enum object_type type;
    union {
        float value;
        char* text;
        function* fp;
        table* tp;
    };
};

extern object null_const;

#define OBJECT_NEW(t, body) \
    object* new_##t(){  \
        object* o=malloc(sizeof(object)); \
        CHECK_ALLOCATION(o); \
        o->type=t_##t;\
        body \
        if(ALLOC_LOG) printf("new " #t "\n");\
        return o; \
    }

#define OBJECT_INIT(t, body) \
    void t##_init (object* o){  \
        o->type=t_##t; \
        body \
    }

#define OBJECT_INIT_NEW(t, body) \
    OBJECT_NEW(t, body) \
    OBJECT_INIT(t, body) \

#define OBJECT_INIT_NEW_DECLARATIONS(t) \
    object* new_##t(); \
    void t##_init(object* o);

OBJECT_INIT_NEW_DECLARATIONS(null)
OBJECT_INIT_NEW_DECLARATIONS(number)
OBJECT_INIT_NEW_DECLARATIONS(function)
OBJECT_INIT_NEW_DECLARATIONS(string)
OBJECT_INIT_NEW_DECLARATIONS(table)

#define CHECK_OBJECT(checked) \
    if(checked==NULL) { ERROR(INCORRECT_OBJECT_POINTER, "Object pointer \"" #checked "\" passed to function %s is null", __FUNCTION__); } \
    if(checked->type<t_null||checked->type>t_table) { ERROR(INCORRECT_OBJECT_POINTER, "Object \"" #checked "\" passed to function %s has incorrect type value %i", __FUNCTION__, checked->type); }

typedef map_t(object) object_map_t;

typedef struct table table;
struct table {
    int ref_count;
    object_map_t fields;
};

typedef enum function_type function_type;
enum function_type {
    f_native,
    f_ast,
    f_bytecode
};
typedef struct function function;
struct function {
    int ref_count;
    function_type ftype;
    union {
        object (*pointer)(object* arguments, int arguments_count);
        // void* should be expression* but I don't want to create cross dependency here
        void* ast_pointer;
        int label;
    };
    void* enviroment;
    vector argument_names;
    int arguments_count;
    object enclosing_scope;
};

void reference(object* o);

void dereference(object* o);
void delete_unreferenced(object* checked);

void object_init(object* o, object_type type);

void object_deinit(object* o);
void object_delete(object* o);

char* stringify_object(object o);
char* stringify(object o);

#endif