#include "stack.h"

void stack_init(stack* s, size_t item_size, int size){
    s->count=0;
    s->item_size=item_size;
    s->size=size;
    s->items=malloc(item_size*size);
    CHECK_ALLOCATION(s->items)
}

void stack_from(stack* s, size_t item_size, void* source, int source_items_count){
    s->count=source_items_count;
    s->item_size=item_size;
    s->size=source_items_count;
    s->items=source;
}

void* stack_get_data(stack* s){
    return s->items;
}

void stack_deinit(stack* s){
    free(s->items);
}

void* stack_index(const stack* s, int index){
    return (char*)s->items + index*s->item_size;
}

void stack_push(stack* s, const void* value){
    if(s->count>=s->size){
        s->size=nearest_power_of_two(s->count+1);
        s->items=realloc(s->items, s->item_size*s->size);
        CHECK_ALLOCATION(s->items)
    }
    memcpy(stack_index(s, s->count), value, s->item_size);
    s->count++;
}

void stack_insert(stack* s, int index, const void* value){
    s->count++;
    if(s->count>=s->size){
        s->size=nearest_power_of_two(s->count+1);
        s->items=realloc(s->items, s->item_size*s->size);
        CHECK_ALLOCATION(s->items)
    }
    for(int i=s->count-1; i>=index+1; i--){
        memcpy(stack_index(s, i), stack_index(s, i-1), s->item_size);
    }
    memcpy(stack_index(s, index+1), value, s->item_size);
}

void stack_delete(stack* s, int index){
    for(int i=index; i<s->count-1; i++){
        memcpy(stack_index(s, i), stack_index(s, i+1), s->item_size);
    }
    s->count-=1;
    if(s->count<s->size/4){
        s->count/=2;
        s->items=realloc(s->items, s->item_size*s->size);
    }
}

void stack_delete_range(stack* s, int start, int end){
    int deleted_count=end-start+1;// 1, 2, 3 ,4, 5 delete(2, 4) should delete 2, 3, 4  result 1, 5
    s->count-=deleted_count;
    for(int i=start; i<s->count; i++){
        memcpy(stack_index(s, i), stack_index(s, i+deleted_count), s->item_size);
    }
    if(s->count<s->size/4){
        s->size/=2;
        s->items=realloc(s->items, s->item_size*s->size);
    }
}

void* stack_pop(stack* s){
    s->count--;
    char* item_position=((char*)s->items) + s->count*s->item_size/sizeof(char);
    // TODO: add free here
    return (void*)item_position;
}

void* stack_top(stack* s){
    char* item_position=((char*)s->items) + (s->count-1)*s->item_size/sizeof(char);
    return (void*)item_position;
}

int stack_count(const stack* s){
    return s->count;
}

bool stack_empty(const stack* s){
    return s->count==0;
}