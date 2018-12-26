#include "object.h"

#define GC_LOG 0
#define ALLOC_LOG 0

const char* OBJECT_TYPE_NAMES[]={
    "null",
    "number",
    "function",
    "string",
    "table"
};

OBJECT_INIT_NEW(null,)
OBJECT_INIT_NEW(number,)
OBJECT_INIT_NEW(function,
    o->fp=malloc(sizeof(function_));
    o->fp->argument_names.items=NULL;
    o->fp->arguments_count=0;
    o->fp->ftype=f_native;
    o->fp->enclosing_scope=NULL;
)
OBJECT_INIT_NEW(string,)
OBJECT_INIT_NEW(table,
    o->tp=malloc(sizeof(table_));
    map_init(&o->tp->fields);
)

object null_const={t_null};

void reference(object* o){
    if(o->type==t_table){
        o->tp->ref_count++;
    } else if(o->type==t_string){
        // maybe it is a dirty hack, will find out later
        // other option would be a copy function
        o->text=strdup(o->text);
    }
}

void dereference(object* o){
    if(o->type==t_table){
        o->tp->ref_count--;
    }
}

// check if object is referenced by anything if not delete it
void delete_unreferenced(object* o){
    if(o->type!=t_table || o->tp->ref_count==0){// if object isn't a table or it is a table and it's ref_count is zero
        if(GC_LOG){
            USING_STRING(stringify(*o),
                printf("%s was garbage collected.\n", str));
        }
        object_delete(o);
    }
}

void object_deinit(object* o){
    CHECK_OBJECT(o);

    switch(o->type){
        case t_string:
        {
            free(o->text);
            break;
        }
        case t_function:
        {
            if(o->fp->enclosing_scope!=NULL){
                delete_unreferenced(o->fp->enclosing_scope);
            }
            break;
        }
        case t_table: 
        {
            if(o->tp->ref_count==0){
                const char *key;
                map_iter_t iter = map_iter(&o->tp->fields);
                while ((key = map_next(&o->tp->fields, &iter))) {
                    object value=(*map_get(&o->tp->fields, key));
                    dereference(o);// dereference contained object, so it can be garbage collected
                    object_deinit(&value);
                }
                map_deinit(&o->tp->fields);
            }
            break;
        }
        default:;
    }
}

void object_delete(object* o){
    object_deinit(o);
    free(o);
}