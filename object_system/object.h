#ifndef OBJECT_H
#define OBJECT_H
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include "../error/error.h"
#include "../macros.h"
#include "../datatypes/vector.h"

typedef enum object_type object_type;
enum object_type{
    t_null,
    t_number,
    t_function,
    t_string,
    t_table,
    t_pointer
};

extern const char* OBJECT_TYPE_NAMES[];// array mapping enum object_type to their names as strings
extern const int OBJECT_TYPE_NAMES_COUNT;

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
        void* p;
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
OBJECT_INIT_NEW_DECLARATIONS(pointer)

#define STRING_OBJECT(name, string_text) object name; string_init(&name); name.text=(char*)string_text;

#define CHECK_OBJECT(checked) \
    if(checked==NULL) { \
        THROW_ERROR(INCORRECT_OBJECT_POINTER, "Object pointer \"" #checked "\" passed to function %s is null", __FUNCTION__); \
    } \
    if(checked->type<t_null||checked->type>t_table) { \
        THROW_ERROR(INCORRECT_OBJECT_POINTER, "Object \"" #checked "\" passed to function %s has incorrect type value %i", __FUNCTION__, checked->type); \
    }

// declaration of function pointer type used in function objects
typedef object (*object_system_function)(object* arguments, int arguments_count);

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
        object_system_function native_pointer;
        void* source_pointer;
    };
    void* enviroment;
    char** argument_names;
    int arguments_count;
    bool variadic;
    object enclosing_scope;
};

object to_string(const char* s);
object to_number(float n);
object to_pointer(void* p);
object to_function(object_system_function f, char** argument_names, int arguments_count);

void reference(object* o);
void object_init(object* o, object_type type);
void dereference(object* o);

char* stringify_object(object o);
char* stringify(object o);

// these functions should be implemented in higher level module
object call_function(function* f, object* arguments, int arguments_count);
void free_function(function* f);

#include "table.h"

#endif