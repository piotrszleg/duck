#include "object.h"

const char* OBJECT_TYPE_NAMES[]={
    #define X(t) #t,
    OBJECT_TYPES
    #undef X
};
const int OBJECT_TYPE_NAMES_COUNT=5;

void garbage_collector_init(GarbageCollector* gc){
    gc->root=NULL;
    gc->state=gcs_inactive;
    gc->allocations_count=0;
}

void gc_object_init(Executor* E, gc_Object* gco){
    GarbageCollector* gc=get_garbage_collector(E);
    gc_Object* gc_root=gc->root;
    gc->allocations_count++;
    
    gco->ref_count=0;
    if(gc->root!=NULL){
        gc->root->previous=gco;
    }
    gco->next=gc_root;
    gco->previous=NULL;
    gc->root=gco;
}

Object wrap_gc_object(gc_Object* o){
    Object wrapped;
    wrapped.type=o->gc_type;
    wrapped.gco=o;
    return wrapped;
}

#define OBJECT_INIT(t) \
    void t##_init (Object* o){  \
        o->type=t_##t; \
    }

OBJECT_INIT(null)
OBJECT_INIT(float)
OBJECT_INIT(int)
OBJECT_INIT(string)
OBJECT_INIT(pointer)

#undef OBJECT_INIT

void function_init(Executor* E, Object* o){
    o->type=t_function;
    o->fp=malloc(sizeof(Function));
    CHECK_ALLOCATION(o->fp);
    gc_object_init(E, o->gco);
    o->gco->gc_type=t_function;
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
    gc_object_init(E, o->gco);
    o->gco->gc_type=t_table;
    table_component_init(o->tp);
}

void coroutine_init(Executor* E, Object* o){
    o->type=t_coroutine;
    o->co=malloc(sizeof(Coroutine));
    CHECK_ALLOCATION(o->co);
    gc_object_init(E, o->gco);
    o->gco->gc_type=t_coroutine;
}

void gc_pointer_init(Executor* E, gc_Pointer* gcp, gc_PointerFreeFunction free){
    gc_object_init(E, &gcp->gco);
    gcp->gco.gc_type=t_gc_pointer;
    gcp->free=free;
    gcp->mark_children=NULL;
    gcp->dereference_children=NULL;
}

Object to_string(const char* s){
    Object o; 
    string_init(&o);
    /* String Object can contain const string as long as it doesn't dereference it.
    If some object takes ownership over the string it will copy it by calling reference function.
    This way strings are allocated dynamically only if it's needed. */
    o.text=(char*)s;
    return o;
}

Object to_int(int n){
    Object o; 
    int_init(&o); 
    o.int_value=n;
    return o;
}

Object to_float(float n){
    Object o; 
    float_init(&o); 
    o.float_value=n;
    return o;
}

Object to_gc_pointer(gc_Pointer* p){
    Object o;
    o.type=t_gc_pointer;
    o.gcp=p;
    return o;
}

Object to_pointer(void* p){
    Object o; 
    pointer_init(&o); 
    o.p=p;
    return o;
}

Object to_function(Executor* E, ObjectSystemFunction f, char** argument_names, int arguments_count){
    Object o;
    function_init(E, &o);
    
    o.fp->native_pointer=f;
    o.fp->argument_names=argument_names;
    o.fp->arguments_count=arguments_count;
    return o;
}

Object null_const={t_null};

bool is_gc_object(Object o){
    return o.type==t_table || o.type==t_function || o.type==t_gc_pointer || o.type==t_coroutine;
}

void gc_object_unchain(Executor* E, gc_Object* o){
    if(get_garbage_collector(E)->root==o){
        get_garbage_collector(E)->root=o->next;
    }
    if(o->previous!=NULL){
        o->previous->next=o->next;
    }
    if(o->next!=NULL){
        o->next->previous=o->previous;
    }
}

void reference(Object* o){
    if(is_gc_object(*o) && o->gco->ref_count!=ALREADY_DESTROYED){
        o->gco->ref_count++;
    } else if(o->type==t_string){
        // maybe it is a dirty hack, will find out later
        // other option would be a copy function
        o->text=strdup(o->text);
        CHECK_ALLOCATION(o->text)
    }
}

void print_allocated_objects(Executor* E){
    gc_Object* o=get_garbage_collector(E)->root;
    while(o){
        Object wrapped={o->gc_type};
        wrapped.gco=o;
        USING_STRING(stringify(E, wrapped),
            printf("%s\tref_count: %i\n", str, o->ref_count))
        o=o->next;
    }
}

bool function_uses_source_pointer(Function* f) {
    return f->ftype==f_bytecode || f->ftype==f_ast;
}

void gc_mark(Object o){
    if(!is_gc_object(o) || o.gco->marked==true){
        return;
    }
    o.gco->marked=true;
    if(o.type==t_table){
        TableIterator it=table_get_iterator(o.tp);
        for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
            gc_mark(i.key);
            gc_mark(i.value);
        }
    } else if (o.type==t_function){
        gc_mark(o.fp->enclosing_scope);
        if(function_uses_source_pointer(o.fp)){
            gc_mark(wrap_gc_object(o.fp->source_pointer));
        }
    } else if(o.type==t_gc_pointer && o.gcp->mark_children!=NULL){
        o.gcp->mark_children(o.gcp);
    }
}

void gc_unmark_all(GarbageCollector* gc){
    gc_Object* o=gc->root;
    while(o){
        o->marked=false;
        o=o->next;
    }
}

void gc_sweep(Executor* E){
    GarbageCollector* gc=get_garbage_collector(E);
    gc_Object* o=gc->root;
    #define FOREACH_UNMARKED_OBJECT(body) \
        o=gc->root; \
        while(o){ \
            gc_Object* next=o->next; \
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
        gc_object_dereference(E, o);
    )
    // reset ref_count for the third pass to work
    FOREACH_UNMARKED_OBJECT(
        o->ref_count=0;
    )
    gc->state=gcs_freeing_memory;
    // free the memory
    FOREACH_UNMARKED_OBJECT(
       gc_object_dereference(E, o);
    )
    gc->state=gcs_inactive;
    #undef FOREACH_UNMARKED_OBJECT
}

bool gc_should_run(GarbageCollector* gc){
    return gc->allocations_count>MAX_ALLOCATIONS;
}

void gc_run(Executor*E, Object* roots, int roots_count){
    gc_unmark_all(get_garbage_collector(E));
    for(int i=0; i<roots_count; i++){
        gc_mark(roots[i]);
    }
    gc_sweep(E);
}

char* gc_text="<garbage collected text>";

void gc_object_dereference(Executor* E, gc_Object* o){
    Object wrapped=wrap_gc_object(o);
    dereference(E, &wrapped);
}

void gc_object_reference(gc_Object* o){
    Object wrapped=wrap_gc_object(o);
    reference(&wrapped);
}

void dereference(Executor* E, Object* o){
    CHECK_OBJECT(o)
    if(is_gc_object(*o)) {
        if(o->gco->ref_count!=ALREADY_DESTROYED){
            o->gco->ref_count--;
            destroy_unreferenced(E, o);
        }
    } else if(o->type==t_string){
        if(get_garbage_collector(E)->state!=gcs_deinitializing){
            free(o->text);
            o->text=gc_text;
        }
    }
}

void destroy_unreferenced(Executor* E, Object* o){
    if(is_gc_object(*o) && o->gco->ref_count<=0 && o->gco->ref_count!=ALREADY_DESTROYED){
        GarbageCollector* gc=get_garbage_collector(E);
        GarbageCollectorState gc_state=gc->state;
        o->gco->ref_count=ALREADY_DESTROYED;
        switch(o->type){
            case t_function:
            {
                if(gc_state!=gcs_freeing_memory){
                    if(function_uses_source_pointer(o->fp)){
                        gc_object_dereference(E, o->fp->source_pointer);
                    }
                    dereference(E, &o->fp->enclosing_scope);
                }
                if(gc_state!=gcs_deinitializing){
                    gc_object_unchain(E, o->gco);
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
                if(gc_state!=gcs_freeing_memory){
                    call_destroy(E, *o);
                    table_dereference_children(E, o->tp);
                }
                if(gc_state!=gcs_deinitializing){
                    gc_object_unchain(E, o->gco);
                    gc->allocations_count--;
                    free_table(o->tp);
                    o->tp=NULL;
                }
                break;
            }
            case t_gc_pointer:
            {
                if(gc_state!=gcs_freeing_memory && o->gcp->dereference_children!=NULL){
                    o->gcp->dereference_children(E, o->gcp);
                }
                if(gc_state!=gcs_deinitializing){
                    gc_object_unchain(E, o->gco);
                    gc->allocations_count--;
                    o->gcp->free(o->gcp);
                    o->gcp=NULL;
                }
            }
            default:;
        }
    }
}

void object_system_init(Executor* E){
    garbage_collector_init(get_garbage_collector(E));
}

void object_system_deinit(Executor* E){
    gc_run(E, NULL, 0);
}