#include "object.h"

#define GC_LOG 0
#define ALLOC_LOG 0

const char* OBJECT_TYPE_NAMES[]={
    #define X(t) #t,
    OBJECT_TYPES
    #undef X
};
const int OBJECT_TYPE_NAMES_COUNT=5;

gc_object* gc_root=NULL;
gc_state_type gc_state=gcs_inactive;
int allocations_count=0;

void gc_object_init(gc_object* gco){
    gco->ref_count=0;
    if(gc_root!=NULL){
        gc_root->previous=gco;
    }
    gco->next=gc_root;
    gco->previous=NULL;
    gc_root=gco;
}

#define OBJECT_NEW(t, body) \
    object* new_##t(){  \
        object* o=malloc(sizeof(object)); \
        CHECK_ALLOCATION(o); \
        o->type=t_##t;\
        body \
        if(ALLOC_LOG) printf("new " #t "\n");\
        return o; \
    }

#define OBJECT_INIT(t, body) \
    void t##_init (object* o){  \
        o->type=t_##t; \
        body \
    }

#define OBJECT_INIT_NEW(t, body) \
    OBJECT_NEW(t, body) \
    OBJECT_INIT(t, body) \


OBJECT_INIT_NEW(null,)
OBJECT_INIT_NEW(number,)
OBJECT_INIT_NEW(function,
    o->fp=malloc(sizeof(function));
    CHECK_ALLOCATION(o->fp);
    gc_object_init(o->gco);
    o->gco->gc_type=t_function;
    o->fp->variadic=false;
    o->fp->argument_names=NULL;
    o->fp->arguments_count=0;
    o->fp->ftype=f_native;
    o->fp->enclosing_scope=null_const;
    o->fp->environment=NULL;
)
OBJECT_INIT_NEW(string,)
OBJECT_INIT_NEW(table,
    o->tp=malloc(sizeof(table));
    CHECK_ALLOCATION(o->tp);
    gc_object_init(o->gco);
    o->gco->gc_type=t_table;
    table_component_init(o->tp);
)
OBJECT_INIT_NEW(pointer,)

OBJECT_INIT_NEW(gc_pointer,
    gc_object_init(o->gco);
    o->gco->gc_type=t_gc_pointer;
)

#undef OBJECT_NEW
#undef OBJECT_INIT
#undef OBJECT_INIT_NEW

object to_string(const char* s){
    object o; 
    string_init(&o);
    /* String object can contain const string as long as it doesn't dereference it.
    If some object takes ownership over the string it will copy it by calling reference function.
    This way strings are allocated dynamically only if it's needed. */
    o.text=(char*)s;
    return o;
}

object to_number(float n){
    object o; 
    number_init(&o); 
    o.value=n;
    return o;
}

object to_gc_pointer(gc_pointer* p){
    object o;
    o.type=t_gc_pointer;
    o.gcp=p;
    return o;
}

object to_pointer(void* p){
    object o; 
    pointer_init(&o); 
    o.p=p;
    return o;
}

object to_function(object_system_function f, char** argument_names, int arguments_count){
    object o;
    function_init(&o);
    
    o.fp->native_pointer=f;
    o.fp->argument_names=argument_names;
    o.fp->arguments_count=arguments_count;
    return o;
}

object null_const={t_null};

bool is_gc_object(object o){
    return o.type==t_table || o.type==t_function;
}

void gc_object_unchain(gc_object* o){
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

void reference(object* o){
    if(is_gc_object(*o) && o->gco->ref_count!=ALREADY_DESTROYED){
        o->gco->ref_count++;
    } else if(o->type==t_string){
        // maybe it is a dirty hack, will find out later
        // other option would be a copy function
        o->text=strdup(o->text);
        CHECK_ALLOCATION(o->text)
    }
}

void print_allocated_objects(executor* Ex){
    gc_object* o=gc_root;
    while(o){
        object wrapped={o->gc_type};
        wrapped.gco=o;
        USING_STRING(stringify(Ex, wrapped),
            printf("%s\tref_count: %i\n", str, o->ref_count))
        o=o->next;
    }
}

void gc_mark(object o){
    if(o.type==t_table){
        o.tp->gco.marked=true;
        table_iterator it=start_iteration(o.tp);
        for(iteration_result i=table_next(&it); !i.finished; i=table_next(&it)) {
            gc_mark(i.key);
            gc_mark(i.value);
        }
    } else if (o.type==t_function){
        o.fp->gco.marked=true;
        gc_mark(o.fp->enclosing_scope);
    }
}

void gc_unmark_all(){
    gc_object* o=gc_root;
    while(o){
        o->marked=false;
        o=o->next;
    }
}

void gc_sweep(executor* Ex){
    gc_object* o=gc_root;
    #define FOREACH_GC_OBJECT(body) \
        o=gc_root; \
        while(o){ \
            gc_object* next=o->next; \
            if(!o->marked){ \
                body \
            } \
            o=next; \
        }

    gc_state=gcs_deinitializing;
    // dereference all children and call their destructors
    FOREACH_GC_OBJECT(
        if(o->ref_count>0){
            o->ref_count=0;
        }
        gc_dereference(Ex, o);
    )
    // reset ref_count for the third pass to work
    FOREACH_GC_OBJECT(
        o->ref_count=0;
    )
    gc_state=gcs_freeing_memory;
    // free the memory
    FOREACH_GC_OBJECT(
       gc_dereference(Ex, o);
    )
    gc_state=gcs_inactive;
    #undef FOREACH_GC_OBJECT
}

bool gc_should_run(){
    return allocations_count>MAX_ALLOCATIONS;
}

void gc_run(executor*Ex, object* roots, int roots_count){
    gc_unmark_all();
    for(int i=0; i<roots_count; i++){
        gc_mark(roots[i]);
    }
    gc_sweep(Ex);
}

char* gc_text="<garbage collected text>";

void gc_dereference(executor* Ex, gc_object* o){
    object wrapped;
    wrapped.type=o->gc_type;
    wrapped.gco=o;
    dereference(Ex, &wrapped);
}

void dereference(executor* Ex, object* o){
    CHECK_OBJECT(o)
    if(is_gc_object(*o)) {
        if(o->gco->ref_count!=ALREADY_DESTROYED){
            o->gco->ref_count--;
            destroy_unreferenced(Ex, o);
        }
    } else if(o->type==t_string){
        if(gc_state!=gcs_deinitializing){
            free(o->text);
            o->text=gc_text;
        }
    }
}

void destroy_unreferenced(executor* Ex, object* o){
    if(is_gc_object(*o) && o->gco->ref_count<=0){
        o->gco->ref_count=ALREADY_DESTROYED;
        switch(o->type){
            case t_function:
            {
                dereference(Ex, &o->fp->enclosing_scope);

                // in freeing memory stage the function environment will free itself
                if(o->fp->environment!=NULL && gc_state!=gcs_freeing_memory){
                    gc_dereference(Ex, (gc_object*)o->fp->environment);
                }
                if(gc_state!=gcs_deinitializing){
                    gc_object_unchain(o->gco);
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
                    call_destroy(Ex, *o);
                    dereference_children_table(Ex, o->tp);
                }
                if(gc_state!=gcs_deinitializing){
                    gc_object_unchain(o->gco);
                    free_table(o->tp);
                    o->tp=NULL;
                }
                break;
            }
            case t_gc_pointer:
            {
                if(gc_state!=gcs_deinitializing){
                    gc_object_unchain(o->gco);
                    o->gcp->destructor(o->gcp);
                    o->gcp=NULL;
                }
            }
            default:;
        }
    }
}

void object_system_init(){
    table_init(&patching_table);
    reference(&patching_table);
}

void object_system_deinit(executor* Ex){
    // patching table might be used by destructors
    gc_run(Ex, &patching_table, 1);

    patching_table.gco->ref_count=0;
    destroy_unreferenced(Ex, &patching_table);
}