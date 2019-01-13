#ifndef STACK_H
#define STACK_H

#define STACK_SIZE 20

#include "string.h"
#include "stdlib.h"

typedef struct stack stack;
struct stack {
    void* items;
    int item_size;
    int top;
};

void stack_init(stack* s, size_t item_size, int size);
void stack_push(stack* s, const void* o);
void* stack_pop(stack* s);
void* stack_top(stack* s);

#endif