#include "objects_vector.h"

void objects_vector_push(vector* stack, Object object){
    vector_push(stack, (const void*)(&object));
}

Object objects_vector_pop(vector* stack){
    void* pop_result=vector_pop(stack);
    Object* object=pop_result;
    return *object;
}

bool objects_vector_delete(Executor* E, vector* v, Object o){
    for(int i=0; i<vector_count(v); i++){
        if(is(E, *(Object*)vector_index(v, i), o)==0){
            vector_delete(v, i);
            return true;
        }
    }
    return false;
}