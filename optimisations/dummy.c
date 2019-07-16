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
        return t_null;
    }
}

void dummy_foreach_children(Executor* E, Dummy* dummy, gc_PointerForeachChildrenCallback callback){
    dummy->gcp.gco.marked=true;
    if(dummy->type==d_or){
        Object wrapped_left=wrap_gc_object((gc_Object*)dummy->or.left);
        callback(E, &wrapped_left);
        Object wrapped_right=wrap_gc_object((gc_Object*)dummy->or.right);
        callback(E, &wrapped_right);
    }
}

void dummy_free(Dummy* dummy){
    if(dummy->type==d_or){
        dummy_free(dummy->or.left);
        dummy_free(dummy->or.right);
    }
    free(dummy);
}

Dummy* new_dummy(Executor* E){
    Dummy* result=malloc(sizeof(Dummy));
    gc_pointer_init(E, (gc_Pointer*)result, (gc_PointerFreeFunction)dummy_free);
    result->gcp.foreach_children=(gc_PointerForeachChildrenFunction)dummy_foreach_children;
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
    } else if(a->type==d_or){
        return dummies_compatible(a->or.left, b) || dummies_compatible(a->or.right, b);
    } else if(b->type==d_or){
        return dummies_compatible(a, b->or.left) || dummies_compatible(a, b->or.right);
    } else {
        return a->id==b->id;
    }
}

bool dummy_contains(const Dummy* a, const Dummy* b){
    if(a->type==d_any || b->type==d_any){
        return true;
    } else if(a->type==d_or){
        return dummy_contains(a->or.left, b) || dummy_contains(a->or.right, b);
    } else if(b->type==d_or){
        return dummy_contains(a, b->or.left) || dummy_contains(a, b->or.right);
    } else {
        return a->id==b->id;
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
            printf("(%s)", OBJECT_TYPE_NAMES[dummy->known_type]);
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
            THROW_ERROR(BYTECODE_ERROR, "Incorrect dummy type %i.", dummy->type);
    }
    //printf("<%i>", dummy->gcp.gco.ref_count);
}

bool dummy_replace(Executor* E, Dummy** dummy, Dummy* to_replace, Dummy* replacement){
    if((*dummy)->type==d_or){
        return dummy_replace(E, &(*dummy)->or.left, to_replace, replacement)
        ||     dummy_replace(E, &(*dummy)->or.right, to_replace, replacement);
    } else if(dummies_equal(*dummy, to_replace)) {
        gc_object_reference((gc_Object*)replacement);
        gc_object_dereference(E, (gc_Object*)*dummy);
        *dummy=replacement;
        return true;
    } else {
        return false;
    }
}