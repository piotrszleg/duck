#ifndef STACK_H
#define STACK_H

#include <stdlib.h>
#include <string.h>
#include "../error/error.h"

typedef struct Stack Stack;
struct Stack {
    void* items;
    void* top;
    int item_size;
    int size;
};

#include "stack.h"

void stack_init(Stack* s, size_t item_size, int size);
void stack_deinit(Stack* s);
void stack_push(Stack* s, const void* value);
void* stack_pop(Stack* s);
void* stack_top(Stack* s);
void stack_allocate(Stack* s, unsigned int additional_size);
void stack_deallocate(Stack* s, unsigned int allocated_size);
int stack_count(const Stack* s);
void* stack_index(Stack* s, unsigned int index);

#endif