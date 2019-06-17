#include "stack.h"

void stack_init(stack* s, size_t item_size, int size){
    s->count=0;
    s->item_size=item_size;
    s->size=size;
    s->items=malloc(item_size*size);
    CHECK_ALLOCATION(s->items)
}

// source needs to be a dynamically allocated memory block of size item_size*source_items_count
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

void* stack_index_checked(const stack* s, int index){
    if(index<0 || index>=s->count){
        return NULL;
    }
    return (char*)s->items + index*s->item_size;
}

int stack_in_bounds(const stack* s, int index){
    return index>=0 && index<s->count;
}

void* stack_index(const stack* s, int index){
    return (char*)s->items + index*s->item_size;
}

void stack_check_upsize(stack* s){
    if(s->count>s->size){
        s->size=nearest_power_of_two(s->count);
        s->items=realloc(s->items, s->item_size*s->size);
        CHECK_ALLOCATION(s->items)
    }
}

void stack_push(stack* s, const void* value){
    s->count++;
    stack_check_upsize(s);
    memcpy(stack_index(s, s->count-1), value, s->item_size);
}

void stack_insert(stack* s, int index, const void* value){
    s->count++;
    stack_check_upsize(s);
    for(int i=s->count-1; i>=index+1; i--){
        memcpy(stack_index(s, i), stack_index(s, i-1), s->item_size);
    }
    memcpy(stack_index(s, index), value, s->item_size);
}

void stack_insert_multiple(stack* s, int index, void* elements, int count){
    s->count+=count;
    stack_check_upsize(s);
    for(int i=s->count-1; i>=index; i--){
        if(i<index+count){
            memcpy(stack_index(s, i), (char*)elements+(i-index)*s->item_size, s->item_size);
        } else {
            memcpy(stack_index(s, i), stack_index(s, i-count), s->item_size);
        }
    }
}

static void stack_check_downsize(stack* s){
    if(s->count<s->size/4 && s->count>0){
        s->size/=2;
        s->items=realloc(s->items, s->item_size*s->size);
    }
}

void stack_delete(stack* s, int index){
    for(int i=index; i<s->count-1; i++){
        memcpy(stack_index(s, i), stack_index(s, i+1), s->item_size);
    }
    s->count-=1;
    stack_check_downsize(s);
}

void stack_clear(stack* s){
    s->count=0;
}

void stack_delete_range(stack* s, int start, int end){
    int deleted_count=end-start+1;
    s->count-=deleted_count;
    for(int i=start; i<s->count; i++){
        memcpy(stack_index(s, i), stack_index(s, i+deleted_count), s->item_size);
    }
    stack_check_downsize(s);
}

void* stack_pop(stack* s){
    stack_check_downsize(s);// downsize is here because we don't want to deallocate the returned pointer
    s->count--;
    char* item_position=((char*)s->items) + s->count*s->item_size/sizeof(char);
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

// for debugging purposes
static void print_int_array(int* array, int size){
    printf("{");
    for(int i=0; i<size-1; i++){
        printf("%i, ", array[i]);
    }
    printf("%i", array[size-1]);
    printf("}");
}

static bool stack_equals_array(stack* s, int* array){
    for(int i=0; i<stack_count(s); i++){
        if(*(int*)stack_index(s, i)!=array[i]){
            return false;
        }
    }
    return true;
}

void stack_tests(){
    printf("TEST: stack_tests\n");
    int array[]={0, 1, 2, 3, 4, 5, 6, 7 ,8, 9};
    int* array_allocated;
    stack s;
    
    #define FILL_STACK \
        array_allocated=malloc(sizeof(array)); \
        memcpy(array_allocated, array, sizeof(array)); \
        stack_from(&s, sizeof(int), array_allocated, sizeof(array)/sizeof(int));

    #define RESET_STACK \
        stack_deinit(&s); \
        FILL_STACK

    #define ASSERT_CONTENT(...) \
    {   int expected_content[]={__VA_ARGS__}; \
        assert(stack_count(&s)==sizeof(expected_content)/sizeof(int)); \
        assert(stack_equals_array(&s, expected_content)); }
    
    FILL_STACK
    ASSERT_CONTENT(0, 1, 2, 3, 4, 5, 6, 7 ,8, 9)

    stack_delete(&s, 0);
    ASSERT_CONTENT(1, 2, 3, 4, 5, 6, 7 ,8, 9)
    RESET_STACK
    stack_delete(&s, 5);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 6, 7 ,8, 9)
    RESET_STACK
    stack_delete(&s, 9);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 5, 6, 7 ,8)
    RESET_STACK
    
    stack_delete_range(&s, 0, 2);
    ASSERT_CONTENT(3, 4, 5, 6, 7 ,8, 9)
    RESET_STACK
    stack_delete_range(&s, 4, 6);
    ASSERT_CONTENT(0, 1, 2, 3, 7 ,8, 9)
    RESET_STACK
    stack_delete_range(&s, 7, 9);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 5, 6)
    RESET_STACK
    stack_delete_range(&s, 0, 9);
    assert(stack_count(&s)==0);
    RESET_STACK

    stack_insert(&s, 0, &array[2]);
    ASSERT_CONTENT(2, 0, 1, 2, 3, 4, 5, 6, 7 ,8, 9)
    RESET_STACK
    stack_insert(&s, 5, &array[3]);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 3, 5, 6, 7 ,8, 9)
    RESET_STACK
    stack_insert(&s, 10, &array[4]);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 4)
    RESET_STACK

    int to_insert[]={1, 2, 3};
    stack_insert_multiple(&s, 0, to_insert, 3);
    ASSERT_CONTENT(1, 2, 3, 0, 1, 2, 3, 4, 5, 6, 7 ,8, 9)
    RESET_STACK
    stack_insert_multiple(&s, 5, to_insert, 3);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 1, 2, 3, 5, 6, 7 ,8, 9)
    RESET_STACK
    stack_insert_multiple(&s, 10, to_insert, 3);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3)
    RESET_STACK

    stack_clear(&s);
    const int tested_count=20;
    for(int i=0; i<tested_count; i++){
        stack_push(&s, &i);
    }
    assert(stack_count(&s)==tested_count);
    for(int i=0; i<tested_count; i++){
        stack_pop(&s);
    }
    assert(stack_count(&s)==0);

    stack_deinit(&s);

    printf("test successful\n");
}