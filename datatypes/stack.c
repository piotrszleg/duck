#include "stack.h"

void stack_init(stack* s, size_t item_size, int count){
    s->top=0;
    s->item_size=item_size;
    s->items=malloc(item_size*count);
}

void stack_push(stack* s, const void* value){
    char* position=((char*)s->items) + s->top*s->item_size/sizeof(char);
    if(s->top>=STACK_SIZE){
        THROW_ERROR(STACK_OVERFLOW, "Too many items were allocated on the stack.");
    }
    memcpy((void*)position, value, s->item_size);
    s->top++;
}

void* stack_pop(stack* s){
    s->top--;
    char* item_position=((char*)s->items) + s->top*s->item_size/sizeof(char);
    // TODO: add free here
    return (void*)item_position;
}

void* stack_top(stack* s){
    char* item_position=((char*)s->items) + (s->top-1)*s->item_size/sizeof(char);
    return (void*)item_position;
}