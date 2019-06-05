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

typedef struct Executor Executor;

#define OBJECT_TYPES \
    X(null) \
    X(number) \
    X(function) \
    X(string) \
    X(table) \
    X(pointer) \
    X(gc_pointer)

typedef enum {
    #define X(type) t_##type,
    OBJECT_TYPES
    #undef X
} ObjectType;

extern const char* OBJECT_TYPE_NAMES[];// array mapping enum ObjectType to their names as strings
extern const int OBJECT_TYPE_NAMES_COUNT;

typedef struct gc_Object gc_Object;
struct gc_Object {
    int ref_count;
    // gc_objects form a double linked list starting from gc_root
    struct gc_Object* previous;
    struct gc_Object* next;

    bool marked;
    ObjectType gc_type;
};
extern gc_Object* gc_root;

typedef enum {
    gcs_inactive,
    gcs_deinitializing,
    gcs_freeing_memory
} gc_StateType;
extern gc_StateType gc_state;
// when allocations_count in gc_object_init is greater than MAX_ALLOCATIONS the garbage collector will be activated
#define MAX_ALLOCATIONS 100
extern int allocations_count;

#define ALREADY_DESTROYED INT_MIN

typedef struct Table Table;
typedef struct Function Function;
typedef struct gc_pointer gc_pointer;

typedef struct {
    ObjectType type;
    union {
        float value;
        char* text;
        void* p;
        gc_pointer* gcp;
        Function* fp;
        Table* tp;
        /* Function, Table and gc_pointer structs have exact same memory layout as gc_Object
           and can be safely casted to it */
        gc_Object* gco;
    };
} Object;

extern Object null_const;

#define OBJECT_INIT_NEW_DECLARATIONS(t) \
    Object* new_##t(); \
    void t##_init(Object* o);

#define X(type) OBJECT_INIT_NEW_DECLARATIONS(type)
OBJECT_TYPES
#undef X

#define STRING_OBJECT(name, string_text) Object name; string_init(&name); name.text=(char*)string_text;

#define CHECK_OBJECT(checked) \
    if(checked==NULL) { \
        THROW_ERROR(INCORRECT_OBJECT_POINTER, "Object pointer \"" #checked "\" passed to Function %s is null", __FUNCTION__); \
    } \
    if(checked->type<t_null||checked->type>t_gc_pointer) { \
        THROW_ERROR(INCORRECT_OBJECT_POINTER, "Object \"" #checked "\" passed to function %s has incorrect type value %i", __FUNCTION__, checked->type); \
    }

// declaration of function pointer type used in function objects
typedef Object (*ObjectSystemFunction)(Executor* E, Object* arguments, int arguments_count);

typedef void (*gc_PointerDestructorFunction)(void*);

typedef struct gc_pointer gc_pointer;
struct gc_pointer {
    gc_Object gco;
    gc_PointerDestructorFunction destructor;
};

typedef enum {
    f_native,
    f_ast,
    f_bytecode,
    f_special
} FunctionType;

struct Function {
    // gc_Object fields
    gc_Object gco;

    FunctionType ftype;
    union {
        ObjectSystemFunction native_pointer;
        void* source_pointer;
        int special_index;
    };
    gc_pointer* environment;
    char** argument_names;
    int arguments_count;
    bool variadic;
    Object enclosing_scope;
};

void print_allocated_objects();
bool is_gc_object(Object o);
void gc_run(Executor* E, Object* roots, int roots_count);
void call_destroy(Executor* E, Object o);

Object to_string(const char* s);
Object to_number(float n);
Object to_pointer(void* p);
Object to_gc_pointer(gc_pointer* p);
Object to_function(ObjectSystemFunction f, char** argument_names, int arguments_count);

void reference(Object* o);
void object_init(Object* o, ObjectType type);
void gc_dereference(Executor* E, gc_Object* o);
void dereference(Executor* E, Object* o);
void destroy_unreferenced(Executor* E, Object* o);

char* stringify_object(Executor* E, Object o);
char* stringify(Executor* E, Object o);

void object_system_init();
void object_system_deinit(Executor* E);

// these functions should be implemented in higher level module
Object call_function(Executor* E, Function* f, Object* arguments, int arguments_count);
void deinit_function(Function* f);

#include "Table.h"

#endif