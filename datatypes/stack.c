#include "stack.h"

void stack_init(stack* s, size_t item_size, int count){
    s->top=0;
    s->item_size=item_size;
    s->items=malloc(item_size*count);
}

void stack_push(stack* s, const void* value){
    char* position=((char*)s->items) + s->top*s->item_size/sizeof(char);
    memcpy((void*)position, value, s->item_size);
    s->top++;
}

void* stack_pop(stack* s){
    s->top--;
    char* item_position=((char*)s->items) + s->top*s->item_size/sizeof(char);
    return (void*)item_position;
}