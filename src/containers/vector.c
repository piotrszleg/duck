#include "vector.h"

void vector_init(vector* v, size_t item_size, int size){
    v->count=0;
    v->item_size=item_size;
    v->size=size;
    v->items=malloc(item_size*size);
    CHECK_ALLOCATION(v->items)
}

// source needs to be a dynamically allocated memory block of size item_size*source_items_count
void vector_from(vector* v, size_t item_size, void* source, int source_items_count){
    v->count=source_items_count;
    v->item_size=item_size;
    v->size=source_items_count;
    v->items=source;
}

void* vector_get_data(vector* v){
    return v->items;
}

void vector_deinit(vector* v){
    free(v->items);
}

void* vector_index(const vector* v, int index){
    return (char*)v->items + index*v->item_size;
}

void* vector_index_checked(const vector* v, int index){
    if(index<0 || index>=v->count){
        return NULL;
    }
    return (char*)v->items + index*v->item_size;
}

bool vector_in_bounds(const vector* v, int index){
    return index>=0 && index<v->count;
}

void vector_check_upsize(vector* v){
    if(v->count>v->size){
        v->size=nearest_power_of_two(v->count);
        v->items=realloc(v->items, v->item_size*v->size);
        CHECK_ALLOCATION(v->items)
    }
}

void vector_push(vector* v, const void* value){
    v->count++;
    vector_check_upsize(v);
    memcpy(vector_index(v, v->count-1), value, v->item_size);
}

void vector_extend(vector* v, int new_count){
    int old_count=v->count;
    v->count=new_count;
    vector_check_upsize(v);
    memset((char*)v->items+old_count*v->item_size, 0, (v->count-old_count)*v->item_size);
}

void pointers_vector_push(vector* v, void* pointer){
    vector_push(v, &pointer);
}

void* pointers_vector_get(vector* v, int index){
    void** pointer=(void**)vector_index(v, index);
    return *pointer;
}

void pointers_vector_set(vector* v, int index, void* value){
    void** pointer=(void**)vector_index(v, index);
    *pointer=value;
}

void vector_insert(vector* v, int index, const void* value){
    v->count++;
    vector_check_upsize(v);
    for(int i=v->count-1; i>=index+1; i--){
        memcpy(vector_index(v, i), vector_index(v, i-1), v->item_size);
    }
    memcpy(vector_index(v, index), value, v->item_size);
}

void vector_insert_multiple(vector* v, int index, void* elements, int count){
    v->count+=count;
    vector_check_upsize(v);
    for(int i=v->count-1; i>=index; i--){
        if(i<index+count){
            memcpy(vector_index(v, i), (char*)elements+(i-index)*v->item_size, v->item_size);
        } else {
            memcpy(vector_index(v, i), vector_index(v, i-count), v->item_size);
        }
    }
}

static void vector_check_downsize(vector* v){
    if(v->count<v->size/4 && v->count>0){
        v->size/=2;
        v->items=realloc(v->items, v->item_size*v->size);
    }
}

void vector_delete(vector* v, int index){
    for(int i=index; i<v->count-1; i++){
        memcpy(vector_index(v, i), vector_index(v, i+1), v->item_size);
    }
    v->count-=1;
    vector_check_downsize(v);
}

void vector_delete_item(vector* v, void* item){
    for(int i=v->count-1; i>=0; i--){
        if(memcmp(vector_index(v, i), item, v->item_size)==0){
            vector_delete(v, i);
            break;
        }
    }
}

int vector_search(vector* v, void* item){
    for(int i=v->count-1; i>=0; i--){
        if(memcmp(vector_index(v, i), item, v->item_size)==0){
            return i;
        }
    }
    return -1;
}

void vector_clear(vector* v){
    v->count=0;
}

void vector_delete_range(vector* v, int start, int end){
    int deleted_count=end-start+1;
    v->count-=deleted_count;
    for(int i=start; i<v->count; i++){
        memcpy(vector_index(v, i), vector_index(v, i+deleted_count), v->item_size);
    }
    vector_check_downsize(v);
}

void* vector_pop(vector* v){
    // TEMPORARY
    // vector_check_downsize(v);// downsize is here because we don't want to deallocate the returned pointer
    v->count--;
    char* item_position=((char*)v->items) + v->count*v->item_size;
    return (void*)item_position;
}

void* vector_top(vector* v){
    char* item_position=((char*)v->items) + (v->count-1)*v->item_size;
    return (void*)item_position;
}

int vector_count(const vector* v){
    return v->count;
}

bool vector_empty(const vector* v){
    return v->count==0;
}

// for debugging purposes
void print_int_array(int* array, int size){
    printf("{");
    for(int i=0; i<size-1; i++){
        printf("%i, ", array[i]);
    }
    printf("%i", array[size-1]);
    printf("}");
}

static bool vector_equals_array(vector* v, int* array){
    for(int i=0; i<vector_count(v); i++){
        if(*(int*)vector_index(v, i)!=array[i]){
            return false;
        }
    }
    return true;
}

void vector_copy(vector* source, vector* destination){
    destination->count=source->count;
    destination->item_size=source->item_size;
    destination->size=source->size;
    memcpy(destination->items, source->items, destination->size*destination->item_size);
}

void vector_tests(){
    printf("TEST: vector_tests\n");
    int array[]={0, 1, 2, 3, 4, 5, 6, 7 ,8, 9};
    int not_in_array=25;
    int* array_allocated;
    vector v;
    
    #define FILL_VECTOR \
        array_allocated=malloc(sizeof(array)); \
        memcpy(array_allocated, array, sizeof(array)); \
        vector_from(&v, sizeof(int), array_allocated, sizeof(array)/sizeof(int));

    #define RESET_VECTOR \
        vector_deinit(&v); \
        FILL_VECTOR

    #define ASSERT_CONTENT(...) \
    {   int expected_content[]={__VA_ARGS__}; \
        assert(vector_count(&v)==sizeof(expected_content)/sizeof(int)); \
        assert(vector_equals_array(&v, expected_content)); }
    
    FILL_VECTOR
    ASSERT_CONTENT(0, 1, 2, 3, 4, 5, 6, 7 ,8, 9)
    for(int i=0; i<vector_count(&v); i++){
        assert(*(int*)vector_index(&v, i)==array[i]);
    }

    vector_delete(&v, 0);
    ASSERT_CONTENT(1, 2, 3, 4, 5, 6, 7 ,8, 9)
    RESET_VECTOR
    vector_delete(&v, 5);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 6, 7 ,8, 9)
    RESET_VECTOR
    vector_delete(&v, 9);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 5, 6, 7 ,8)
    RESET_VECTOR
    
    vector_delete_range(&v, 0, 2);
    ASSERT_CONTENT(3, 4, 5, 6, 7 ,8, 9)
    RESET_VECTOR
    vector_delete_range(&v, 4, 6);
    ASSERT_CONTENT(0, 1, 2, 3, 7 ,8, 9)
    RESET_VECTOR
    vector_delete_range(&v, 7, 9);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 5, 6)
    RESET_VECTOR
    vector_delete_range(&v, 0, 9);
    assert(vector_count(&v)==0);
    RESET_VECTOR

    vector_delete_item(&v, &array[0]);
    ASSERT_CONTENT(1, 2, 3, 4, 5, 6, 7, 8, 9)
    RESET_VECTOR
    vector_delete_item(&v, &array[3]);
    ASSERT_CONTENT(0, 1, 2, 4, 5, 6, 7, 8, 9)
    RESET_VECTOR
    vector_delete_item(&v, &array[9]);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 5, 6, 7, 8)
    RESET_VECTOR
    vector_delete_item(&v, &not_in_array);
    ASSERT_CONTENT(1, 2, 3, 4, 5, 6, 7, 8, 9)
    RESET_VECTOR

    vector_insert(&v, 0, &array[2]);
    ASSERT_CONTENT(2, 0, 1, 2, 3, 4, 5, 6, 7 ,8, 9)
    RESET_VECTOR
    vector_insert(&v, 5, &array[3]);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 3, 5, 6, 7 ,8, 9)
    RESET_VECTOR
    vector_insert(&v, 10, &array[4]);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 4)
    RESET_VECTOR

    int to_insert[]={1, 2, 3};
    vector_insert_multiple(&v, 0, to_insert, 3);
    ASSERT_CONTENT(1, 2, 3, 0, 1, 2, 3, 4, 5, 6, 7 ,8, 9)
    RESET_VECTOR
    vector_insert_multiple(&v, 5, to_insert, 3);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 1, 2, 3, 5, 6, 7 ,8, 9)
    RESET_VECTOR
    vector_insert_multiple(&v, 10, to_insert, 3);
    ASSERT_CONTENT(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3)
    RESET_VECTOR

    vector_clear(&v);
    const int tested_count=100;
    for(int i=0; i<tested_count; i++){
        vector_push(&v, &i);
    }
    assert(vector_count(&v)==tested_count);
    for(int i=0; i<tested_count; i++){
        vector_pop(&v);
    }
    assert(vector_count(&v)==0);

    vector_deinit(&v);

    printf("test successful\n");
}