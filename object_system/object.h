#ifndef OBJECT_H
#define OBJECT_H
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include "../error/error.h"
#include "../utility.h"
#include "../datatypes/vector.h"

// TODO: Change this declaration to X macro
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

typedef struct gc_object gc_object;
struct gc_object {
    int ref_count;
    // gc_objects form a double linked list starting from gc_root
    struct gc_object* previous;
    struct gc_object* next;

    bool marked;
    object_type gc_type;
};
extern gc_object* gc_root;

typedef enum gc_state_type gc_state_type;
enum gc_state_type {
    gcs_inactive,
    gcs_calling_destructors,
    gcs_freeing_memory
};
extern gc_state_type gc_state;
// when allocations_count in gc_object_init is greater than MAX_ALLOCATIONS the garbage collector will be activated
#define MAX_ALLOCATIONS 100
extern int allocations_count;

#define ALREADY_DESTROYED INT_MIN

typedef struct table table;
typedef struct function function;

typedef struct object object;
struct object{
    enum object_type type;
    union {
        float value;
        char* text;
        void* p;
        function* fp;
        table* tp;
        /* function and table structs have exact same memory layout as gc_object
           and can be safely casted to it */
        gc_object* gco;
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
    if(checked->type<t_null||checked->type>t_pointer) { \
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
    // gc_object fields
    gc_object gco;

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

void print_allocated_objects();
bool is_gc_object(object o);
void gc_run(object* roots, int roots_count);
void call_destroy(object o);

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