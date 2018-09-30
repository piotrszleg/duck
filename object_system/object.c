#include "object.h"

const char* OBJECT_TYPE_NAMES[]={
    "null",
    "number",
    "function",
    "string",
    "table"
};

#define RUNTIME_OBJECT_NEW(t, body) \
    t* new_ ## t(){  \
        t* instance=malloc(sizeof(t)); \
		instance->type=t_ ## t; \
        body \
        return instance; \
    }

// all objects should be made and removed using these functions

RUNTIME_OBJECT_NEW(null,)
RUNTIME_OBJECT_NEW(number,)
RUNTIME_OBJECT_NEW(function,
    vector_init(&instance->argument_names);
)
RUNTIME_OBJECT_NEW(string,)
RUNTIME_OBJECT_NEW(table,
    map_init(&((table*)instance)->fields);
)

// TODO free pointers
void object_delete(object* o){
    switch(o->type){
        case t_string:
        {
            string* as_string=(string*)o;
            free(as_string);
            free(as_string->value);
            break;
        }
        case t_number:
            free((number*)o);
            break;
        case t_function:
            free((table*)o);
            break;
        case t_table: 
        {
            table* as_table=(table*)o;
            map_deinit(&as_table->fields);
            free(as_table);
            break;
        }
        default:
            free(o);
    }
}

int is_number(const char *s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return 0;
    }
    return 1;
}

// TODO:  write tests
int is_falsy(object* o){
    switch(o->type){
        case t_null:
            return 0;// null is falsy
        case t_function:
            return 1;// every function isn't falsy
        case t_string:
            return strlen(((string*)o)->value);// "" (string of length 0) is falsy
        case t_number:
            return ((number*)o)->value==0;// 0 is falsy
        case t_table:
            return ((table*)o)->fields.base.nnodes==0;// empty table is falsy
    }
}

object* cast(object* o, object_type type){
    if(o->type==type){
        return o;
    }
    switch(type){
        case t_string:
            {
                char* buffer=malloc(sizeof(char)*1024);
                strncpy(buffer, stringify(o), 1024);
                string* result=new_string();
                result->value=buffer;
                return (object*)result;
            }
        case t_number:
            {
                number* result=new_number();
                if(o->type==t_null){
                    result->value=0;// null is zero
                    return (object*)result;
                } else if(o->type==t_string && is_number(((string*)o)->value)){
                    result->value=atoi(((string*)o)->value);// convert string to int if it contains number
                    return (object*)result;
                }
            }
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Can't convert from <%s> to <%s>", OBJECT_TYPE_NAMES[o->type], OBJECT_TYPE_NAMES[type]);
            return (object*)new_null();// conversion can't be performed
    }
}

object* operator(object* a, object *b, char* op){
    if(a->type!=b->type){
        b=cast(b, a->type);
    }
    switch(a->type){
        case t_string:
            if(strcmp(op, "+")==0){
                char* buffer=malloc(sizeof(char)*1024);
                if(!buffer){ ERROR(MEMORY_ALLOCATION_FAILURE, "Memory allocation failure"); }
                strcpy(buffer, ((string*) a)->value);
                strcat(buffer, ((string*) b)->value);
                string* result=new_string();
                result->value=buffer;
                return (object*)result;
            }

        case t_number:
            {
                number* result=new_number();
                if(strcmp(op, "+")==0){
                    result->value=((number*) a)->value + ((number*) b)->value;
                    return (object*)result;
                }
                if(strcmp(op, "-")==0){
                    result->value=((number*) a)->value - ((number*) b)->value;
                    return (object*)result;
                }
                if(strcmp(op, "*")==0){
                    result->value=((number*) a)->value * ((number*) b)->value;
                    return (object*)result;
                }
                if(strcmp(op, "/")==0){
                    result->value=((number*) a)->value / ((number*) b)->value;
                    return (object*)result;
                }
            }
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Can't perform operotion '%s' object of type <%s> and <%s>", op, OBJECT_TYPE_NAMES[a->type], OBJECT_TYPE_NAMES[b->type]);
            return NULL;

    }
}

char* stringify(struct object* o){
    switch(o->type){
        case t_string:
            return ((string*)o)->value;
        case t_number:
            {
                char buffer[100];
                return itoa(((number*)o)->value, buffer, 10);
            }
        case t_table:
            {
                table* t=(table*)o;
                char* result=calloc(100, sizeof(char));
                const char *key;
                map_iter_t iter = map_iter(&m);
                strcat(result, "[");
                int first=1;
                while ((key = map_next(&t->fields, &iter))) {
                    if(first) {
                        first=0;
                    } else {
                        strcat(result, ", ");
                    }
                    strcat(result, key);
                    strcat(result, "=");
                    strcat(result, stringify(*map_get(&t->fields, key)));
                    
                }
                strcat(result, "]");
                return result;
            }
        case t_function:
            return "<function>";
        case t_null:
            return "<null>";
        default:
            ERROR(INCORRECT_OBJECT_POINTER, "Object at %#8x has no valid type value", (unsigned int)o);
            return NULL;
    }
}

object* call(object* o, table* arguments){
    switch(o->type){
        case t_function:
            return ((struct function*)o)->pointer(o, arguments);
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Object of type <%s> is not callable", OBJECT_TYPE_NAMES[o->type]);
            return NULL;
    }
}

object* get(object* o, char*key){
    switch(o->type){
        case t_table:
        {
            object** map_get_result=map_get(&((struct table*)o)->fields, key);

            if(map_get_result==NULL){// there's no object at this key
                return (object*)new_null();
            }else {
                return *map_get_result;
            }
        }
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Can't index object of type <%s>", OBJECT_TYPE_NAMES[o->type]);
            return NULL;
    }
}

void set(object* o, char*key, object* value){
    switch(o->type){
        case t_table:
        {
            map_set(&((struct table*)o)->fields, key, value);
            break;
        }
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Can't index object of type <%s>", OBJECT_TYPE_NAMES[o->type]);
    }
}
