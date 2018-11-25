#ifndef STACK_H
#define STACK_H

#define STACK_SIZE 10

#include "string.h"
#include "stdlib.h"

typedef struct stack stack;
struct stack {
    void* items;
    int item_size;
    int top;
};

void stack_init(stack* s, size_t item_size, int size);
void stack_push(stack* stack, void* o);
void* stack_pop(stack* stack);

#endif