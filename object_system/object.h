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
    X(int) \
    X(float) \
    X(function) \
    X(string) \
    X(table) \
    X(pointer) \
    X(managed_pointer) \
    X(coroutine)

typedef enum {
    #define X(type) t_##type,
    OBJECT_TYPES
    #undef X
} ObjectType;

#define LAST_OBJECT_TYPE t_coroutine

extern const char* OBJECT_TYPE_NAMES[];// array mapping enum ObjectType to their names as strings
extern const int OBJECT_TYPE_NAMES_COUNT;

typedef struct HeapObject HeapObject;
struct HeapObject {
    int ref_count;
    // heap_objects form a double linked list starting from gc_root
    struct HeapObject* previous;
    struct HeapObject* next;

    bool marked;
    ObjectType gc_type;
};

typedef enum {
    gcs_inactive,
    gcs_deinitializing,
    gcs_freeing_memory
} GarbageCollectorState;

// difference between survivors_count and allocations_count at which the garbage collection should start
#define MAX_ALLOCATIONS_INCREASE 100
#define ALREADY_DESTROYED -5

typedef struct {
    HeapObject* root;
    GarbageCollectorState state;
    int survivors_count;// how many objects survived last garbage collection
    int allocations_count;
} GarbageCollector;

void garbage_collector_init(GarbageCollector*);

// forward declarations of allocated object components
typedef struct Table Table;
typedef struct Function Function;
typedef struct Coroutine Coroutine;
typedef struct ManagedPointer ManagedPointer;

typedef struct {
    ObjectType type;
    union {
        float float_value;
        int int_value;
        char* text;
        void* p;
        ManagedPointer* gcp;
        Function* fp;
        Table* tp;
        Coroutine* co;
        /* ManagedPointer, Function, Table and Coroutine structs have same memory layout as HeapObject
           and can be safely casted to it */
        HeapObject* gco;
    };
} Object;

extern Object null_const;

#define OBJECT_INIT(t) \
    void t##_init (Object* o);

#define OBJECT_INIT_E(t) \
    void t##_init (Executor* E, Object* o);

OBJECT_INIT(null)
OBJECT_INIT(int)
OBJECT_INIT(float)
OBJECT_INIT(string)
OBJECT_INIT(pointer)

OBJECT_INIT_E(coroutine)
OBJECT_INIT_E(function)
OBJECT_INIT_E(table)

#undef OBJECT_INIT
#undef OBJECT_INIT_E

#define STRING_OBJECT(name, string_text) Object name; string_init(&name); name.text=(char*)string_text;

#define CHECK_OBJECT(checked) \
    if(checked==NULL) { \
        THROW_ERROR(INCORRECT_OBJECT_POINTER, "Object pointer \"" #checked "\" passed to function %s is null", __FUNCTION__); \
    } \
    if(checked->type<t_null||checked->type>LAST_OBJECT_TYPE) { \
        THROW_ERROR(INCORRECT_OBJECT_POINTER, "Object \"" #checked "\" passed to function %s has incorrect type value %i", __FUNCTION__, checked->type); \
    }

// declaration of function pointer type used in function objects
typedef Object (*ObjectSystemFunction)(Executor* E, Object scope, Object* arguments, int arguments_count);

typedef void (*ManagedPointerFreeFunction)(ManagedPointer*);
typedef void (*ManagedPointerForeachChildrenCallback)(Executor*, Object*);
typedef void (*ManagedPointerForeachChildrenFunction)(Executor* E, ManagedPointer*, ManagedPointerForeachChildrenCallback);

typedef struct ManagedPointer ManagedPointer;
struct ManagedPointer {
    HeapObject gco;
    ManagedPointerFreeFunction free;
    ManagedPointerForeachChildrenFunction foreach_children;
};

void managed_pointer_init(Executor* E, ManagedPointer* gcp, ManagedPointerFreeFunction free);

typedef enum {
    f_native,
    f_ast,
    f_bytecode,
    f_special
} FunctionType;

struct Function {
    HeapObject gco;

    FunctionType ftype;
    union {
        ObjectSystemFunction native_pointer;
        HeapObject* source_pointer;
        unsigned special_index;
    };
    char** argument_names;
    unsigned arguments_count;
    bool variadic;
    Object enclosing_scope;
};

struct Coroutine {
    HeapObject gco;
    Executor* executor;
    enum State {
        co_uninitialized,
        co_running,
        co_finished
    } state;
};

void print_allocated_objects(Executor* E);
bool is_heap_object(Object o);
bool gc_should_run(GarbageCollector* gc);
void gc_unmark_all(GarbageCollector* gc);
void gc_mark(Executor* E, Object* o);
void gc_sweep(Executor* E);
Object wrap_heap_object(HeapObject* o);
void call_destroy(Executor* E, Object o);

Object to_string(const char* s);
Object to_int(int n);
Object to_float(float n);
Object to_pointer(void* p);
Object to_managed_pointer(ManagedPointer* p);
Object to_function(Executor* E, ObjectSystemFunction f, char** argument_names, int arguments_count);

void reference(Object* o);
void object_init(Object* o, ObjectType type);
void heap_object_dereference(Executor* E, HeapObject* o);
void heap_object_reference(HeapObject* o);
void dereference(Executor* E, Object* o);
void destroy_unreferenced(Executor* E, Object* o);

char* stringify_object(Executor* E, Object o);
char* stringify(Executor* E, Object o);

void object_system_init(Executor* E);
void object_system_deinit(Executor* E);

// these functions should be implemented in higher level module
Object call_function(Executor* E, Function* f, Object* arguments, int arguments_count);
Object call_coroutine(Executor* E, Coroutine* coroutine, Object* arguments, int arguments_count);
void coroutine_free(Coroutine* co);
void coroutine_foreach_children(Executor* E, Coroutine* co, ManagedPointerForeachChildrenCallback);
Object executor_on_unhandled_error(Executor* E, Object error);
GarbageCollector* executor_get_garbage_collector(Executor*);
Object executor_get_patching_table(Executor*);

#include "table.h"

#endif