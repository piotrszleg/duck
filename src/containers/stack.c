#include "stack.h"

void stack_init(Stack* s, size_t item_size, int size){
    s->item_size=item_size;
    s->size=size;
    if(size*item_size>0){
        s->items=malloc(item_size*size);
        CHECK_ALLOCATION(s->items);
        s->top=s->items-item_size;
    }
}

void stack_deinit(Stack* s){
    free(s->items);
}

void stack_push(Stack* s, const void* value){
    s->top+=s->item_size;
    memcpy(s->top, value, s->item_size);
}

void* stack_pop(Stack* s){
    s->top-=s->item_size;
    return s->top;
    /*
    POP(t)
        get_stack s
        ld t s
        t-=sizeof(Object)
        st s t
    */   
}


void* stack_top(Stack* s){
    return s->top;
}

int stack_count(const Stack* s){
    return (s->top+s->item_size-s->items)/s->item_size;
}

void* stack_index(Stack* s, unsigned int index){
    return s->items+index*s->item_size;
}

void stack_allocate(Stack* s, unsigned int additional_size) {
    s->size+=additional_size;
    int top_offset=s->top-s->items;
    s->items=realloc(s->items, s->item_size*s->size);
    s->top=s->items+top_offset;
}

void stack_deallocate(Stack* s, unsigned int allocated_size) {
    s->size-=allocated_size;
    int top_offset=s->top-s->items;
    s->items=realloc(s->items, s->item_size*s->size);
    s->top=s->items+top_offset;
}