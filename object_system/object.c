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
    gc_Object* gc_root=get_garbage_collector(E)->root;
    gco->ref_count=0;
    if(gc_root!=NULL){
        gc_root->previous=gco;
    }
    gco->next=gc_root;
    gco->previous=NULL;
    gc_root=gco;
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
OBJECT_INIT(number)
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
    o->fp->environment=NULL;
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

void table_gc_pointer(Executor* E, Object* o){
    o->type=t_gc_pointer;
    gc_object_init(E, o->gco);
    o->gco->gc_type=t_gc_pointer;
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

Object to_number(float n){
    Object o; 
    number_init(&o); 
    o.value=n;
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
    gc_Object* gc_root=get_garbage_collector(E)->root;
    if(gc_root==o){
        gc_root=o->next;
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

void gc_mark(Object o){
    if(o.type==t_table){
        o.tp->gco.marked=true;
        TableIterator it=start_iteration(o.tp);
        for(IterationResult i=table_next(&it); !i.finished; i=table_next(&it)) {
            gc_mark(i.key);
            gc_mark(i.value);
        }
    } else if (o.type==t_function){
        o.fp->gco.marked=true;
        gc_mark(o.fp->enclosing_scope);
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
    #define FOREACH_GC_OBJECT(body) \
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
    FOREACH_GC_OBJECT(
        if(o->ref_count>0){
            o->ref_count=0;
        }
        gc_dereference(E, o);
    )
    // reset ref_count for the third pass to work
    FOREACH_GC_OBJECT(
        o->ref_count=0;
    )
    gc->state=gcs_freeing_memory;
    // free the memory
    FOREACH_GC_OBJECT(
       gc_dereference(E, o);
    )
    gc->state=gcs_inactive;
    #undef FOREACH_GC_OBJECT
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

void gc_dereference(Executor* E, gc_Object* o){
    Object wrapped=wrap_gc_object(o);
    dereference(E, &wrapped);
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
    GarbageCollectorState gc_state=get_garbage_collector(E)->state;
    if(is_gc_object(*o) && o->gco->ref_count<=0){
        o->gco->ref_count=ALREADY_DESTROYED;
        switch(o->type){
            case t_function:
            {
                dereference(E, &o->fp->enclosing_scope);

                // in freeing memory stage the function environment will free itself
                if(o->fp->environment!=NULL && gc_state!=gcs_freeing_memory){
                    gc_dereference(E, (gc_Object*)o->fp->environment);
                }
                if(gc_state!=gcs_deinitializing){
                    gc_object_unchain(E, o->gco);
                    if(o->fp->argument_names!=NULL){
                        for(int i=0; i<o->fp->arguments_count; i++){
                            free(o->fp->argument_names[i]);
                        }
                    }
                    deinit_function(o->fp);
                    free(o->fp);
                    o->fp=NULL;
                }
                break;
            }
            case t_table:
            {
                if(gc_state!=gcs_freeing_memory){
                    call_destroy(E, *o);
                    dereference_children_table(E, o->tp);
                }
                if(gc_state!=gcs_deinitializing){
                    gc_object_unchain(E, o->gco);
                    free_table(o->tp);
                    o->tp=NULL;
                }
                break;
            }
            case t_gc_pointer:
            {
                if(gc_state!=gcs_deinitializing){
                    gc_object_unchain(E, o->gco);
                    o->gcp->destructor(o->gcp);
                    o->gcp=NULL;
                }
            }
            default:;
        }
    }
}

void object_system_init(Executor* E){
    table_init(E, &patching_table);
    reference(&patching_table);
}

void object_system_deinit(Executor* E){
    // patching table might be used by destructors
    gc_run(E, &patching_table, 1);

    patching_table.gco->ref_count=0;
    destroy_unreferenced(E, &patching_table);
}