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

#define executor void

#define OBJECT_TYPES \
    X(null) \
    X(number) \
    X(function) \
    X(string) \
    X(table) \
    X(pointer) \
    X(gc_pointer)

// TODO: Change this declaration to X macro
typedef enum object_type object_type;
enum object_type{
    #define X(type) t_##type,
    OBJECT_TYPES
    #undef X
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
    gcs_deinitializing,
    gcs_freeing_memory
};
extern gc_state_type gc_state;
// when allocations_count in gc_object_init is greater than MAX_ALLOCATIONS the garbage collector will be activated
#define MAX_ALLOCATIONS 100
extern int allocations_count;

#define ALREADY_DESTROYED INT_MIN

typedef struct table table;
typedef struct function function;
typedef struct gc_pointer gc_pointer;

typedef struct object object;
struct object{
    enum object_type type;
    union {
        float value;
        char* text;
        void* p;
        gc_pointer* gcp;
        function* fp;
        table* tp;
        /* function, table and gc_pointer structs have exact same memory layout as gc_object
           and can be safely casted to it */
        gc_object* gco;
    };
};

extern object null_const;

#define OBJECT_INIT_NEW_DECLARATIONS(t) \
    object* new_##t(); \
    void t##_init(object* o);

#define X(type) OBJECT_INIT_NEW_DECLARATIONS(type)
OBJECT_TYPES
#undef X

#define STRING_OBJECT(name, string_text) object name; string_init(&name); name.text=(char*)string_text;

#define CHECK_OBJECT(checked) \
    if(checked==NULL) { \
        THROW_ERROR(INCORRECT_OBJECT_POINTER, "Object pointer \"" #checked "\" passed to function %s is null", __FUNCTION__); \
    } \
    if(checked->type<t_null||checked->type>t_gc_pointer) { \
        THROW_ERROR(INCORRECT_OBJECT_POINTER, "Object \"" #checked "\" passed to function %s has incorrect type value %i", __FUNCTION__, checked->type); \
    }

// declaration of function pointer type used in function objects
typedef object (*object_system_function)(executor* Ex, object* arguments, int arguments_count);

typedef void (*gc_pointer_destructor)(void*);

typedef struct gc_pointer gc_pointer;
struct gc_pointer {
    gc_object gco;
    gc_pointer_destructor destructor;
};

typedef enum function_type function_type;
enum function_type {
    f_native,
    f_ast,
    f_bytecode,
    f_special
};

typedef struct function function;
struct function {
    // gc_object fields
    gc_object gco;

    function_type ftype;
    union {
        object_system_function native_pointer;
        void* source_pointer;
        int special_index;
    };
    gc_pointer* environment;
    char** argument_names;
    int arguments_count;
    bool variadic;
    object enclosing_scope;
};

void print_allocated_objects();
bool is_gc_object(object o);
void gc_run(executor* Ex, object* roots, int roots_count);
void call_destroy(executor* Ex, object o);

object to_string(const char* s);
object to_number(float n);
object to_pointer(void* p);
object to_gc_pointer(gc_pointer* p);
object to_function(object_system_function f, char** argument_names, int arguments_count);

void reference(object* o);
void object_init(object* o, object_type type);
void gc_dereference(executor* Ex, gc_object* o);
void dereference(executor* Ex, object* o);
void destroy_unreferenced(executor* Ex, object* o);

char* stringify_object(executor* Ex, object o);
char* stringify(executor* Ex, object o);

void object_system_init();
void object_system_deinit(executor* Ex);

// these functions should be implemented in higher level module
object call_function(executor* Ex, function* f, object* arguments, int arguments_count);
void deinit_function(function* f);

#include "table.h"

#endif