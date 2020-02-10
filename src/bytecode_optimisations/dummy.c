#include "dummy.h"

bool dummy_is_typed(const Dummy* dummy){
    return dummy->type==d_known_type || dummy->type==d_constant;
}

ObjectType dummy_type(const Dummy* dummy){
    if(dummy->type==d_known_type){
        return dummy->known_type;
    } else if(dummy->type==d_constant){
        return dummy->constant_value.type;
    } else {
        return t_any;
    }
}

void dummy_foreach_children(Executor* E, Dummy* dummy, ForeachChildrenCallback callback, void* data){
    if(dummy->type==d_constant){
        callback(E, &dummy->constant_value, data);
    }
    if(dummy->type==d_or){
        Object wrapped_left=wrap_heap_object((HeapObject*)dummy->or.left);
        callback(E, &wrapped_left, data);
        Object wrapped_right=wrap_heap_object((HeapObject*)dummy->or.right);
        callback(E, &wrapped_right, data);
    }
}

void dummy_free(Dummy* dummy){
    free(dummy);
}

static inline Dummy* new_dummy(Executor* E, DummyType type){
    Dummy* result=malloc(sizeof(Dummy));
    managed_pointer_init(E, (ManagedPointer*)result, (ManagedPointerFreeFunction)dummy_free);
    result->mp.foreach_children=(ManagedPointerForeachChildrenFunction)dummy_foreach_children;
    result->type=type;
    return result;
}

Dummy* new_any_dummy(Executor* E){
    return new_dummy(E, d_any);
}

Dummy* new_any_type_dummy(Executor* E, unsigned* id_counter){
    Dummy* result=new_dummy(E, d_any_type);
    result->id=*id_counter;
    (*id_counter)++;
    return result;
}

Dummy* new_known_type_dummy(Executor* E, ObjectType known_type, unsigned* id_counter){
    Dummy* result=new_dummy(E, d_known_type);
    result->known_type=known_type;
    result->id=*id_counter;
    (*id_counter)++;
    return result;
}

Dummy* new_constant_dummy(Executor* E, Object constant_value, unsigned* id_counter){
    Dummy* result=new_dummy(E, d_constant);
    reference(&constant_value);
    result->constant_value=constant_value;
    result->id=*id_counter;
    (*id_counter)++;
    return result;
}

Dummy* new_or_dummy(Executor* E, Dummy* left, Dummy* right, unsigned* id_counter){
    Dummy* result=new_dummy(E, d_or);
    dummy_reference(left);
    result->or.left=left;
    dummy_reference(right);
    result->or.right=right;
    result->id=*id_counter;
    (*id_counter)++;
    return result;
}

bool dummies_equal(const Dummy* a, const Dummy* b){
    if(a->type==d_any || b->type==d_any){
        return true;
    } else {
        return a->id==b->id;
    }
}

bool dummies_compatible(const Dummy* a, const Dummy* b){
    if(a->type==d_any || b->type==d_any){
        return true;
    } else if(a->id==b->id) {
        return true;
    } else if(a->type==d_or){
        return dummies_compatible(a->or.left, b) || dummies_compatible(a->or.right, b);
    } else if(b->type==d_or){
        return dummies_compatible(a, b->or.left) || dummies_compatible(a, b->or.right);
    } else {
        return false;
    }
}

bool dummy_contains(const Dummy* a, const Dummy* b){
    if(a->type==d_any || b->type==d_any){
        return true;
    } else if(a->id==b->id){
        return true;
    } else if(a->type==d_or){
        return dummy_contains(a->or.left, b) || dummy_contains(a->or.right, b);
    } else {
        return false;
    }
}

static void print_id(unsigned int n){
    int interval='Z'-'A';
    do{
        printf("%c", 'A'+n%interval);
        n/=interval;
    } while(n);
}

void dummy_print(const Dummy* dummy){
    switch(dummy->type){
        case d_any:
            printf("any");
            break;
        case d_any_type:
            print_id(dummy->id);
            break;
        case d_known_type:
            printf("(%s)", get_type_name(dummy->known_type));
            print_id(dummy->id);
            break;
        case d_constant:
            USING_STRING(stringify_object(NULL, dummy->constant_value),
                printf("%s", str));
            break;
        case d_or:
            printf("(");
            dummy_print(dummy->or.left);
            printf(" or ");
            dummy_print(dummy->or.right);
            printf(")");
            break;
        default:
            INCORRECT_ENUM_VALUE(DummyType, dummy, dummy->type);
    }
}

bool dummy_replace(Executor* E, Dummy** dummy, Dummy* to_replace, Dummy* replacement){
    if((*dummy)->type==d_or){
        return dummy_replace(E, &(*dummy)->or.left, to_replace, replacement)
        ||     dummy_replace(E, &(*dummy)->or.right, to_replace, replacement);
    } else if(dummies_equal(*dummy, to_replace)) {
        dummy_reference(replacement);
        dummy_dereference(E, *dummy);
        *dummy=replacement;
        return true;
    } else {
        return false;
    }
}

void dummy_reference(Dummy* dummy){
    heap_object_reference((HeapObject*)dummy);
}

void dummy_dereference(Executor* E, Dummy* dummy){
    heap_object_dereference(E, (HeapObject*)dummy);
}

/* 
This function might seem nonsensical in terms of correct program logic,
because if ref_count of an object is lesser then zero it should be deallocated
and the pointer to it should be invalid. Also if dummy type has incorrect value
then probably either its memory was corrupted or it wasn't initialized correctly.
However the real purpose behind it is to find memory corruption errors early on 
in the debugger.
*/
void dummy_assert_correctness(Dummy* dummy){
    assert(dummy->mp.hp.ref_count>=1);
    switch (dummy->type)
    {
        case d_any:
        case d_any_type:
        case d_known_type:
            break;
        case d_constant:
            if(is_heap_object(dummy->constant_value)){
                assert(dummy->constant_value.hp->ref_count>=1);
            }
            break;
        case d_or:
            dummy_assert_correctness(dummy->or.left);
            dummy_assert_correctness(dummy->or.left);
            break;
        default:
            // dummy type has incorrect value
            assert(false);
    }
}