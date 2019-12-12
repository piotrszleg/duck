#ifndef DUMMY_H
#define DUMMY_H

#include <stdbool.h>
#include <assert.h>
#include "../object_system/object.h"

typedef enum {
    d_any, // we don't care about value in this dummy at all
    // for the rest id is used to check if two dummies are the same 
    d_any_type, 
    d_known_type, // uses known_type from union
    d_constant, // uses constant_value from union
    d_or // used to merge two dummies into one, uses or from union
} DummyType;

typedef struct Dummy Dummy;
struct Dummy {
    ManagedPointer mp;
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

Dummy* new_any_dummy(Executor* E);
Dummy* new_any_type_dummy(Executor* E, unsigned* id_counter);
Dummy* new_known_type_dummy(Executor* E, ObjectType known_type, unsigned* id_counter);
Dummy* new_constant_dummy(Executor* E, Object constant_value, unsigned* id_counter);
Dummy* new_or_dummy(Executor* E, Dummy* left, Dummy* right, unsigned* id_counter);

bool dummy_is_typed(const Dummy* dummy);
ObjectTypeOrUnknown dummy_type(const Dummy* dummy);
bool dummies_equal(const Dummy* a, const Dummy* b);
bool dummies_compatible(const Dummy* a, const Dummy* b);
bool dummy_contains(const Dummy* a, const Dummy* b);
void dummy_print(const Dummy* dummy);
// returns true if replacement happened
bool dummy_replace(Executor* E, Dummy** dummy, Dummy* to_replace, Dummy* replacement);
void dummy_reference(Dummy* dummy);
void dummy_dereference(Executor* E, Dummy* dummy);
void dummy_assert_correctness(Dummy* dummy);

#endif