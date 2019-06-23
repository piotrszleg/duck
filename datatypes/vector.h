#ifndef VECTOR_H
#define VECTOR_H

#define VECTOR_INITIAL_SIZE 20

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "../error/error.h"
#include "../utility.h"

typedef struct vector vector;
struct vector {
    void* items;
    int item_size;
    int count;
    int size;
};

void vector_init(vector* v, size_t item_size, int size);
void vector_from(vector* v, size_t item_size, void* source, int source_size);
void vector_deinit(vector* v);

void vector_push(vector* v, const void* o);
void* vector_pop(vector* v);
void* vector_top(vector* v);
// will make vector count equal to new_count, added elements are zeroed
void vector_extend(vector* v, int new_count);

int vector_count(const vector* v);
bool vector_empty(const vector* v);
int vector_search(vector* v, void* item);

void vector_insert(vector* v, int index, const void* value);
void vector_insert_multiple(vector* v, int index, void* elements, int count);

void vector_delete(vector* v, int index);
void vector_delete_range(vector* v, int start, int end);
void vector_clear(vector* v);
// searches for item starting from the end and deletes it
// will only delete one item
// if item is not found in the vector first item will be deleted
void vector_delete_item(vector* v, void* item);

void* vector_index(const vector* v, int index);
void* vector_index_checked(const vector* v, int index);
bool vector_in_bounds(const vector* v, int index);

void* vector_get_data(vector* v);
void vector_copy(vector* source, vector* destination);

void pointers_vector_push(vector* v, void* pointer);
void* pointers_vector_get(vector* v, int index);
void pointers_vector_set(vector* v, int index, void* value);

void vector_tests();

#endif