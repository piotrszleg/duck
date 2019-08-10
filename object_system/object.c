#include "object.h"

static const char* OBJECT_TYPE_NAMES[]={
    #define X(t) #t,
    OBJECT_TYPES
    #undef X
};

static inline GarbageCollector* executor_get_garbage_collector(Executor* E){
    return ((ObjectSystem*)E)->gc;
} 

void garbage_collector_init(GarbageCollector* gc){
    gc->root=NULL;
    gc->state=gcs_inactive;
    gc->survivors_count=0;
    gc->allocations_count=0;
}

void heap_object_init(Executor* E, HeapObject* hp){
    GarbageCollector* gc=executor_get_garbage_collector(E);
    HeapObject* gc_root=gc->root;
    gc->allocations_count++;
    
    hp->ref_count=0;
    if(gc->root!=NULL){
        gc->root->previous=hp;
    }
    hp->next=gc_root;
    hp->previous=NULL;
    gc->root=hp;
}

Object wrap_heap_object(HeapObject* o){
    Object wrapped;
    wrapped.type=o->gc_type;
    wrapped.hp=o;
    return wrapped;
}

Object new_symbol(Executor* E, char* comment) {
    Object result;
    result.type=t_symbol;
    result.sp=malloc(sizeof(Symbol));
    CHECK_ALLOCATION(result.sp);
    heap_object_init(E, result.hp);
    
    result.hp->gc_type=t_symbol;
    unsigned* symbols_counter=&((ObjectSystem*)E)->symbols_counter;
    result.sp->index=*symbols_counter;
    (*symbols_counter)++;
    result.sp->comment=strdup(comment);
    return result;
}

void function_init(Executor* E, Object* o){
    o->type=t_function;
    o->fp=malloc(sizeof(Function));
    CHECK_ALLOCATION(o->fp);
    heap_object_init(E, o->hp);
    o->hp->gc_type=t_function;
    o->fp->variadic=false;
    o->fp->argument_names=NULL;
    o->fp->arguments_count=0;
    o->fp->ftype=f_native;
    o->fp->enclosing_scope=null_const;
}

void table_init(Executor* E, Object* o){
    o->type=t_table;
    o->tp=malloc(sizeof(Table));
    CHECK_ALLOCATION(o->tp);
    heap_object_init(E, o->hp);
    o->hp->gc_type=t_table;
    table_component_init(o->tp);
}

void coroutine_init(Executor* E, Object* o){
    o->type=t_coroutine;
    o->co=malloc(sizeof(Coroutine));
    CHECK_ALLOCATION(o->co);
    heap_object_init(E, o->hp);
    o->hp->gc_type=t_coroutine;
}

void managed_pointer_init(Executor* E, ManagedPointer* mp, ManagedPointerFreeFunction free){
    heap_object_init(E, &mp->hp);
    mp->hp.gc_type=t_managed_pointer;
    mp->free=free;
    mp->foreach_children=NULL;
}

Object to_string(const char* s){
    Object o; 
    o.type=t_string;
    /* String Object can contain const string as long as it doesn't dereference it.
    If some object takes ownership over the string it will copy it by calling reference function.
    This way strings are allocated dynamically only if it's needed. */
    o.text=(char*)s;
    return o;
}

Object to_int(int n){
    Object o; 
    o.type=t_int;
    o.int_value=n;
    return o;
}

Object to_float(float n){
    Object o; 
    o.type=t_float;
    o.float_value=n;
    return o;
}

Object to_pointer(void* p){
    Object o; 
    o.type=t_pointer;
    o.p=p;
    return o;
}

Object to_native_function(Executor* E, ObjectSystemFunction f, char** argument_names, int arguments_count, bool variadic) {
    Object o;
    function_init(E, &o);
    
    o.fp->native_pointer=f;
    o.fp->argument_names=argument_names;
    o.fp->arguments_count=arguments_count;
    o.fp->variadic=variadic;
    return o;
}

Object null_const={t_null};

bool is_heap_object(Object o){
    return o.type==t_table || o.type==t_function || o.type==t_managed_pointer || o.type==t_coroutine || o.type==t_symbol;
}

void heap_object_unchain(Executor* E, HeapObject* o){
    if(executor_get_garbage_collector(E)->root==o){
        executor_get_garbage_collector(E)->root=o->next;
    }
    if(o->previous!=NULL){
        o->previous->next=o->next;
    }
    if(o->next!=NULL){
        o->next->previous=o->previous;
    }
}

void reference(Object* o){
    if(is_heap_object(*o) && o->hp->ref_count!=ALREADY_DESTROYED){
        o->hp->ref_count++;
    } else if(o->type==t_string){
        // maybe it is a dirty hack, will find out later
        // other option would be a copy function
        o->text=strdup(o->text);
        CHECK_ALLOCATION(o->text)
    }
}

void print_allocated_objects(Executor* E){
    HeapObject* o=executor_get_garbage_collector(E)->root;
    while(o){
        Object wrapped={o->gc_type};
        wrapped.hp=o;
        USING_STRING(stringify(E, wrapped),
            printf("%s\tref_count: %i\n", str, o->ref_count))
        o=o->next;
    }
}

bool function_uses_source_pointer(Function* f) {
    return f->ftype==f_bytecode || f->ftype==f_ast;
}

void gc_mark(Executor* E, Object* o){
    if(!is_heap_object(*o) || o->hp->marked==true){
        return;
    }
    o->hp->marked=true;
    switch(o->type){
    case t_table:{
        table_foreach_children(E, o->tp, gc_mark);
        break;
    }
    case t_function:
        gc_mark(E, &o->fp->enclosing_scope);
        if(function_uses_source_pointer(o->fp)){
            Object wrapped=wrap_heap_object(o->fp->source_pointer);
            gc_mark(E, &wrapped);
        }
        break;
    case t_managed_pointer:
        if(o->mp->foreach_children!=NULL){
            o->mp->foreach_children(E, o->mp, gc_mark);
        }
        break;
    case t_coroutine:
        if(o->mp->foreach_children!=NULL){
            coroutine_foreach_children(E, o->co, gc_mark);
        }
        break;
    default:;
    }
}

void gc_unmark_all(GarbageCollector* gc){
    HeapObject* o=gc->root;
    HeapObject* previous=o;
    while(o){
        o->marked=false;
        previous=o;
        o=o->next;
    }
}

void gc_sweep(Executor* E){
    GarbageCollector* gc=executor_get_garbage_collector(E);
    HeapObject* o=gc->root;
    #define FOREACH_UNMARKED_OBJECT(body) \
        o=gc->root; \
        while(o){ \
            HeapObject* next=o->next; \
            if(!o->marked){ \
                body \
            } \
            o=next; \
        }

    gc->state=gcs_deinitializing;
    // dereference all children and call their destructors
    FOREACH_UNMARKED_OBJECT(
        if(o->ref_count>0){
            o->ref_count=0;
        }
        heap_object_dereference(E, o);
    )
    // reset ref_count for the third pass to work
    FOREACH_UNMARKED_OBJECT(
        o->ref_count=0;
    )
    gc->state=gcs_freeing_memory;
    // free the memory
    FOREACH_UNMARKED_OBJECT(
       heap_object_dereference(E, o);
    )
    gc->state=gcs_inactive;
    gc->survivors_count=gc->allocations_count;
    #undef FOREACH_UNMARKED_OBJECT
}

bool gc_should_run(GarbageCollector* gc){
    return gc->allocations_count-gc->survivors_count>MAX_ALLOCATIONS_INCREASE;
}

char* gc_text="<garbage collected text>";

void heap_object_dereference(Executor* E, HeapObject* o){
    Object wrapped=wrap_heap_object(o);
    dereference(E, &wrapped);
}

void heap_object_reference(HeapObject* o){
    Object wrapped=wrap_heap_object(o);
    reference(&wrapped);
}

void dereference(Executor* E, Object* o){
    if(is_heap_object(*o)) {
        if(o->hp->ref_count!=ALREADY_DESTROYED){
            o->hp->ref_count--;
            destroy_unreferenced(E, o);
        }
    } else if(o->type==t_string){
        if(executor_get_garbage_collector(E)->state!=gcs_deinitializing){
            free(o->text);
            o->text=gc_text;
        }
    }
}

void destroy_unreferenced(Executor* E, Object* o){
    if(is_heap_object(*o) && o->hp->ref_count<=0 && o->hp->ref_count!=ALREADY_DESTROYED){
        GarbageCollector* gc=executor_get_garbage_collector(E);
        GarbageCollectorState gc_state=gc->state;
        o->hp->ref_count=ALREADY_DESTROYED;
        // during garbage collection objects are dereferenced two times
        // first their children are dereferenced and then their memory is freed
        // this allows calling destructors correctly
        // if objects are dereferenced normally (without garbage collector running)
        // both of these actions are performed at one call
        bool dereferencing_children=gc_state==gcs_inactive||gc_state==gcs_deinitializing;
        bool freeing_memory=gc_state==gcs_inactive||gc_state==gcs_freeing_memory;
        switch(o->type){
            case t_function:
            {
                if(dereferencing_children){
                    if(function_uses_source_pointer(o->fp)){
                        heap_object_dereference(E, o->fp->source_pointer);
                    }
                    dereference(E, &o->fp->enclosing_scope);
                }
                if(freeing_memory){
                    heap_object_unchain(E, o->hp);
                    if(o->fp->argument_names!=NULL){
                        for(int i=0; i<o->fp->arguments_count; i++){
                            free(o->fp->argument_names[i]);
                        }
                    }
                    gc->allocations_count--;
                    free(o->fp);
                    o->fp=NULL;
                }
                break;
            }
            case t_table:
            {
                if(dereferencing_children){
                    call_destroy(E, *o);
                    table_foreach_children(E, o->tp, dereference);
                }
                if(freeing_memory){
                    heap_object_unchain(E, o->hp);
                    gc->allocations_count--;
                    table_free(o->tp);
                    o->tp=NULL;
                }
                break;
            }
            case t_managed_pointer:
            {
                if(dereferencing_children && o->mp->foreach_children!=NULL){
                    o->mp->foreach_children(E, o->mp, dereference);
                }
                if(freeing_memory){
                    heap_object_unchain(E, o->hp);
                    gc->allocations_count--;
                    o->mp->free(o->mp);
                    o->mp=NULL;
                }
                break;
            }
            case t_coroutine:
            {
                if(dereferencing_children){
                    coroutine_foreach_children(E, o->co, dereference);
                }
                if(freeing_memory){
                    heap_object_unchain(E, o->hp);
                    gc->allocations_count--;
                    coroutine_free(o->co);
                    o->co=NULL;
                }
                break;
            }
            case t_symbol:
            {
                if(freeing_memory){
                    heap_object_unchain(E, o->hp);
                    gc->allocations_count--;
                    free(o->sp->comment);
                    o->sp=NULL;
                }
                break;
            }
            default:;
        }
    }
}

void object_system_init(Executor* E){
    ObjectSystem* executor=OBJECT_SYSTEM(E);
    executor->gc=malloc(sizeof(GarbageCollector));
    garbage_collector_init(executor->gc);
    executor->symbols_counter=0;
    table_init(E, &executor->overrides_table);
    reference(&executor->overrides_table);
    table_init(E, &executor->types_table);
    reference(&executor->types_table);

    #define X(name) \
        executor->builtin_symbols.name=new_symbol(E, #name); \
        table_set(E, executor->overrides_table.tp, to_string(#name), executor->builtin_symbols.name);
    OVERRIDES
    #undef X

    #define X(name) \
        executor->type_symbols[t_##name]=new_symbol(E, #name); \
        table_set(E, executor->types_table.tp, to_string(#name), executor->type_symbols[t_##name]);
    OBJECT_TYPES
    #undef X
     
}

Object get_type_symbol(Executor* E, ObjectType type){
    return OBJECT_SYSTEM(E)->type_symbols[type];
}

const char* get_type_name(ObjectType type){
    return OBJECT_TYPE_NAMES[type];
}

void object_system_deinit(Executor* E){
    // free all allocated objects
    gc_unmark_all(executor_get_garbage_collector(E));
    gc_sweep(E);
    free(OBJECT_SYSTEM(E)->gc);
}