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
#include "../c_fixes.h"
#include "../datatypes/vector.h"

#define OVERRIDES \
    X(operator) \
    X(cast) \
    X(compare) \
    X(hash) \
    X(iterator) \
    X(serialize) \
    X(stringify) \
    X(call) \
    X(get) \
    X(set) \
    X(copy) \
    X(destroy) \
    X(prototype) \
    X(is_error)

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
    X(coroutine) \
    X(symbol)

typedef enum {
    #define X(type) t_##type,
    OBJECT_TYPES
    #undef X
} ObjectType;

#define LAST_OBJECT_TYPE t_symbol

typedef struct HeapObject HeapObject;
struct HeapObject {
    int ref_count;
    // heap_objects form a double linked list starting from gc_root
    struct HeapObject* previous;
    struct HeapObject* next;

    bool marked;
    bool attached;

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
typedef struct Symbol Symbol;

typedef struct {
    ObjectType type;
    union {
        float float_value;
        int int_value;
        char* text;
        void* p;
        ManagedPointer* mp;
        Function* fp;
        Table* tp;
        Coroutine* co;
        Symbol* sp;
        /* ManagedPointer, Function, Table and Coroutine structs have same memory layout as HeapObject
           and can be safely casted to it */
        HeapObject* hp;
    };
} Object;

typedef struct {
    GarbageCollector* gc;
    unsigned symbols_counter;
    struct {
        #define X(name) Object name;
        OVERRIDES
        #undef X
    } builtin_symbols;
    uint last_builtin_symbol;
    Object overrides_table;
    Object types_table;
    Object type_symbols[LAST_OBJECT_TYPE+1];
} ObjectSystem;

// ObjectSystem struct should be the first field of executor
#define OBJECT_SYSTEM(executor) ((ObjectSystem*)executor)
#define OVERRIDE(executor, override_name) OBJECT_SYSTEM(executor)->builtin_symbols.override_name

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
OBJECT_INIT_E(symbol)

#undef OBJECT_INIT
#undef OBJECT_INIT_E

typedef struct ManagedPointer ManagedPointer;

typedef void (*ManagedPointerFreeFunction)(ManagedPointer*);
typedef void (*ManagedPointerForeachChildrenCallback)(Executor*, Object*);
typedef void (*ManagedPointerForeachChildrenFunction)(Executor* E, ManagedPointer*, ManagedPointerForeachChildrenCallback);
typedef ManagedPointer* (*ManagedPointerCopyFunction)(Executor* E, const ManagedPointer*);

struct ManagedPointer {
    HeapObject hp;
    ManagedPointerCopyFunction copy;
    ManagedPointerFreeFunction free;
    ManagedPointerForeachChildrenFunction foreach_children;
};

void managed_pointer_init(Executor* E, ManagedPointer* mp, ManagedPointerFreeFunction free);

typedef enum {
    f_native,
    f_ast,
    f_bytecode,
    f_special
} FunctionType;

// declaration of function pointer type used in function objects
typedef Object (*ObjectSystemFunction)(Executor* E, Object scope, Object* arguments, int arguments_count);

struct Function {
    HeapObject hp;

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
    HeapObject hp;
    Executor* executor;
    enum State {
        co_uninitialized,
        co_running,
        co_finished
    } state;
};

struct Symbol {
    HeapObject hp;
    unsigned index;
    char* comment;
};

void print_allocated_objects(Executor* E);
bool is_heap_object(Object o);
bool gc_should_run(GarbageCollector* gc);
void gc_unmark_all(GarbageCollector* gc);
void gc_mark(Executor* E, Object* o);
void gc_sweep(Executor* E);
Object wrap_heap_object(HeapObject* o);
void call_destroy(Executor* E, Object o);

Object get_type_symbol(Executor* E, ObjectType type);
const char* get_type_name(ObjectType type);

Object to_string(const char* s);
Object to_int(int n);
Object to_float(float n);
Object to_pointer(void* p);
Object to_native_function(Executor* E, ObjectSystemFunction f, char** argument_names, int arguments_count, bool variadic);
Object new_symbol(Executor* E, char* comment);

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
bool symbol_is_builtin(Executor* E, Symbol* sp);

void attach(Executor* E, Object* o);
void detach(Executor* E, Object* o);

// these functions should be implemented in higher level module
Object call_function(Executor* E, Function* f, Object* arguments, int arguments_count);
Object call_coroutine(Executor* E, Coroutine* coroutine, Object* arguments, int arguments_count);
void coroutine_free(Coroutine* co);
void coroutine_foreach_children(Executor* E, Coroutine* co, ManagedPointerForeachChildrenCallback);
Object executor_on_unhandled_error(Executor* E, Object error);
Object executor_get_patching_table(Executor*);

#include "table.h"

#endif