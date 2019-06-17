#ifndef STACK_H
#define STACK_H

#define STACK_INITIAL_SIZE 20

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "../error/error.h"
#include "../utility.h"

typedef struct stack stack;
struct stack {
    void* items;
    int item_size;
    int count;
    int size;
};

void stack_init(stack* s, size_t item_size, int size);
void stack_from(stack* s, size_t item_size, void* source, int source_size);
void stack_deinit(stack* s);
void stack_push(stack* s, const void* o);
void stack_insert(stack* s, int index, const void* value);
void stack_insert_multiple(stack* s, int index, void* elements, int count);
void stack_delete(stack* s, int index);
void stack_clear(stack* s);
void stack_delete_range(stack* s, int start, int end);
void* stack_index(const stack* s, int index);
void* stack_index_checked(const stack* s, int index);
bool stack_in_bounds(const stack* s, int index);
void* stack_pop(stack* s);
void* stack_top(stack* s);
int stack_count(const stack* s);
bool stack_empty(const stack* s);
void* stack_get_data(stack* s);
void stack_tests();

#endif