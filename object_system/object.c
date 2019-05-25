#include "object.h"

#define GC_LOG 0
#define ALLOC_LOG 0

const char* OBJECT_TYPE_NAMES[]={
    "null",
    "number",
    "function",
    "string",
    "table",
    "pointer"
};
const int OBJECT_TYPE_NAMES_COUNT=5;

OBJECT_INIT_NEW(null,)
OBJECT_INIT_NEW(number,)
OBJECT_INIT_NEW(function,
    o->fp=malloc(sizeof(function));
    CHECK_ALLOCATION(o->fp);
    o->fp->ref_count=0;
    o->fp->variadic=false;
    o->fp->argument_names=NULL;
    o->fp->arguments_count=0;
    o->fp->ftype=f_native;
    o->fp->enclosing_scope=null_const;
)
OBJECT_INIT_NEW(string,)
OBJECT_INIT_NEW(table,
    o->tp=malloc(sizeof(table));
    CHECK_ALLOCATION(o->tp);
    o->tp->ref_count=0;
    table_component_init(o->tp);
)
OBJECT_INIT_NEW(pointer,)

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

void reference(object* o){
    if(o->type==t_table){
        o->tp->ref_count++;
    } else if(o->type==t_function){
        o->fp->ref_count++;
    } else if(o->type==t_string){
        // maybe it is a dirty hack, will find out later
        // other option would be a copy function
        o->text=strdup(o->text);
        CHECK_ALLOCATION(o->text);
    }
}

void delete_table(table* t);

#define ALREADY_DESTROYED INT_MIN
object call_destroy(object o);
void dereference(object* o){
    CHECK_OBJECT(o);

    switch(o->type){
        case t_string:
        {
            free(o->text);
            break;
        }
        case t_function:
        {
            if(o->fp->ref_count<=1 && o->tp->ref_count!=ALREADY_DESTROYED){
                o->fp->ref_count=ALREADY_DESTROYED;// make sure that there won't be a cycle of scope dereferencing function
                dereference(&o->fp->enclosing_scope);
                if(o->fp->argument_names!=NULL){
                    for(int i=0; i<o->fp->arguments_count; i++){
                        free(o->fp->argument_names[i]);
                    }
                }
                free_function(o->fp);
            } else {
                o->fp->ref_count--;
            }
            break;
        }
        case t_table: 
        {
            if(o->tp->ref_count<=1 && o->tp->ref_count!=ALREADY_DESTROYED){
                call_destroy(*o);
                o->tp->ref_count=ALREADY_DESTROYED;
                delete_table(o->tp);
            } else {
                o->tp->ref_count--;
            }
            break;
        }
        default:;
    }
}
#undef ALREADY_DESTROYED