#ifndef DUMMY_H
#define DUMMY_H

#include <stdbool.h>
#include "../object_system/object.h"

typedef enum {
    d_any,
    d_any_type,
    d_known_type,
    d_constant,
    d_or
} DummyType;

typedef struct Dummy Dummy;
struct Dummy {
    gc_Pointer gcp;
    unsigned int id;
    DummyType type;
    union {
        ObjectType known_type;
        Object constant_value;
        struct {
            Dummy* left;
            Dummy* right;
        } or;
    };
};

Dummy* new_dummy(Executor* E);
bool dummy_is_typed(const Dummy* dummy);
ObjectType dummy_type(const Dummy* dummy);
bool dummies_equal(const Dummy* a, const Dummy* b);
bool dummies_compatible(const Dummy* a, const Dummy* b);
bool dummy_contains(const Dummy* a, const Dummy* b);
void dummy_print(const Dummy* dummy);
// returns true if replacement happened
bool dummy_replace(Executor* E, Dummy** dummy, Dummy* to_replace, Dummy* replacement);

#endif