#include "object.h"

#define GC_LOG 0
#define ALLOC_LOG 0
#define STRINGIFY_BUFFER_SIZE 100

const char* OBJECT_TYPE_NAMES[]={
    "null",
    "number",
    "function",
    "string",
    "table"
};

// creates string variable str, executes body and frees the string afterwards
#define USING_STRING(string_expression, body) { char* str=string_expression; body; free(str); }

// all objects should be made and removed using these functions

RUNTIME_OBJECT_NEW(null,)
RUNTIME_OBJECT_NEW(number,)
RUNTIME_OBJECT_NEW(function,
    instance->arguments_count=0;
    instance->ftype=f_native;
    instance->enclosing_scope=NULL;
)
RUNTIME_OBJECT_NEW(string,)
RUNTIME_OBJECT_NEW(table,
    map_init(&((table*)instance)->fields);
)

// check if object is referenced by anything if not delete it
void delete_unreferenced(object* o){
    if(o->ref_count==0){
        if(GC_LOG){
            USING_STRING(stringify(o),    
                printf("%s was garbage collected.\n", str));
        }
        object_delete(o);
    }
}

// TODO free pointers
void object_delete(object* o){
    CHECK_OBJECT(o);

    switch(o->type){
        case t_string:
        {
            string* as_string=(string*)o;
            free(as_string->value);
            free(as_string);
            break;
        }
        case t_number:
            free((number*)o);
            break;
        case t_function:
        {
            function* as_function=(function*)o;
            if(as_function->enclosing_scope!=NULL){
                delete_unreferenced((object*)as_function->enclosing_scope);
            }
            free(as_function);
            break;
        }
        case t_table: 
        {
            table* as_table=(table*)o;
            const char *key;
            map_iter_t iter = map_iter(&as_table->fields);
            while ((key = map_next(&as_table->fields, &iter))) {
                object* value=(*map_get(&as_table->fields, key));
                value->ref_count--;// dereference contained object, so it can be garbage collected
                delete_unreferenced(value);
            }
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
    CHECK_OBJECT(o);

    switch(o->type){
        case t_null:
            return 1;// null is falsy
        case t_function:
            return 0;// every function isn't falsy
        case t_string:
            return strlen(((string*)o)->value)==0;// "" (string of length 0) is falsy
        case t_number:
            return ((number*)o)->value==0;// 0 is falsy
        case t_table:
            return ((table*)o)->fields.base.nnodes==0;// empty table is falsy
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Incorrect object pointer passed to is_falsy function.");
    }
}

int sign(int x){
    return (x > 0) - (x < 0);
}

// if a>b returns 1 if a<b returns -1, if a==b returns 0
int compare(object* a, object *b){
    CHECK_OBJECT(a);
    CHECK_OBJECT(b);
    // null is always less than anything
    if(a->type!=t_null && b->type==t_null){
        return 1;
    }
    if(b->type!=t_null && a->type==t_null){
        return -1;
    }
    if(b->type==t_null && a->type==t_null){
        return 0;
    }
    switch(a->type){
        case t_string:
            if(a->type==b->type){
                return sign(strlen(((string*)a)->value)-strlen(((string*)b)->value));
            }
        case t_number:
            if(a->type==b->type){
                return sign(((number*)a)->value-((number*)b)->value);
            }
        case t_table:
            if(a->type==b->type){
                return sign(((table*)a)->fields.base.nnodes-((table*)b)->fields.base.nnodes);
            }
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Can't compare <%s> to <%s>", OBJECT_TYPE_NAMES[a->type], OBJECT_TYPE_NAMES[b->type]);
    }
}

object* cast(object* o, object_type type){
    CHECK_OBJECT(o);
    if(o->type==type){
        return o;
    }
    switch(type){
        case t_string:
        {
            string* result=new_string();
            result->value=stringify(o);
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
        case t_function:
        {
            object* called=o;
            do{
                // recursively search for object of type function under key "call" inside of 'o' object structure
                called=get(called, "call");
                // get will fail at some point so it shouldn't create an infinite loop
            } while(called->type!=t_function);
            return called;
        }
        default:
            ERROR(TYPE_CONVERSION_FAILURE, "Can't convert from <%s> to <%s>", OBJECT_TYPE_NAMES[o->type], OBJECT_TYPE_NAMES[type]);
            return (object*)new_null();// conversion can't be performed
    }
}

number* create_number(int value){
    number* n=new_number();
    n->value=value;
    return n;
}

// TODO replace create_number with new_number with value argument
// replace ifs with a switch
object* operator(object* a, object *b, char* op){
    CHECK_OBJECT(a);
    CHECK_OBJECT(b);
    if(strcmp(op, "==")==0){
        return (object*)create_number(compare(a, b)==0);
    }
    if(strcmp(op, "!=")==0){
        return (object*)create_number(compare(a, b)!=0);
    }
    if(strcmp(op, ">")==0){
        return (object*)create_number(compare(a, b)==1);
    }
    if(strcmp(op, "<")==0){
        return (object*)create_number(compare(a, b)==-1);
    }
    if(strcmp(op, ">=")==0){
        int comparison_result=compare(a, b);
        return (object*)create_number(comparison_result==1||comparison_result==0);
    }
    if(strcmp(op, "<=")==0){
        int comparison_result=compare(a, b);
        return (object*)create_number(comparison_result==-1||comparison_result==0);
    }
    if(strcmp(op, "||")==0){
        if(!is_falsy(a)){
            return a;
        } else if (!is_falsy(b)){
            return b;
        } else {
            return (object*)new_null();
        }
    }
    if(strcmp(op, "&&")==0){
        if(is_falsy(a)){
            return a;
        } else if (is_falsy(b)){
            return b;
        } else{
            return b;
        }
    }
    if(strcmp(op, "!")==0){
        return (object*)create_number(is_falsy(a));
    }
    if(strcmp(op, "-")==0 && b->type==t_null){
        number* a_as_number=(number*)cast(a, t_number);
        a_as_number->value=-a_as_number->value;
        return (object*)a_as_number;
    }
    if(a->type!=b->type){
        b=cast(b, a->type);
    }
    switch(a->type){
        case t_string:
            if(strcmp(op, "+")==0){
                char* buffer=malloc(sizeof(char)*1024);
                CHECK_ALLOCATION(buffer);
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
            return (object*)new_null();

    }
}

char* stringify(object* o){
    CHECK_OBJECT(o);
    switch(o->type){
        case t_string:
            return strdup(((string*)o)->value);
        case t_number:
            {
                float n=((number*)o)->value;
                char* buffer=calloc(STRINGIFY_BUFFER_SIZE, sizeof(char));
                CHECK_ALLOCATION(buffer);
                int ceiled=n;
                if(((float)ceiled)==n){
                    sprintf(buffer,"%d",ceiled);// stringify number as an integer
                } else{
                    sprintf(buffer,"%f",n);// stringify number as an float
                }
                char* buffer_truncated=strdup(buffer);
                free(buffer);
                return buffer_truncated;
            }
        case t_table:
            {
                table* t=(table*)o;
                char* buffer=calloc(STRINGIFY_BUFFER_SIZE, sizeof(char));
                CHECK_ALLOCATION(buffer);
                const char *key;
                map_iter_t iter = map_iter(&m);
                strcat(buffer, "[");
                int first=1;
                while ((key = map_next(&t->fields, &iter))) {
                    object* value=*map_get(&t->fields, key);
                    if(first) {
                        first=0;
                    } else {
                        strcat(buffer, ", ");
                    }
                    strcat(buffer, key);
                    strcat(buffer, "=");
                    if (value!=o){
                        USING_STRING(stringify(value), 
                            strcat(buffer, str));
                    } else {
                        strcat(buffer, "self");// cycling reference
                    }
                }
                strcat(buffer, "]");
                char* buffer_truncated=strdup(buffer);
                free(buffer);
                return buffer_truncated;
            }
        case t_function:
            return strdup("<function>");
        case t_null:
            return strdup("<null>");
        default:
            ERROR(INCORRECT_OBJECT_POINTER, "Object at %#8x has no valid type value", (unsigned int)o);
            return strdup("<INCORRECT_OBJECT_POINTER>");
    }
}

object* get(object* o, char* key){
    CHECK_OBJECT(o);
    switch(o->type){
        case t_table:
        {
            // try to get "get" operator overriding function from the table and use it
            object** map_get_override=map_get(&((struct table*)o)->fields, "get");
            if(map_get_override!=NULL){
                // create arguments for the function
                function* get_function=(function*)cast(*map_get_override, t_function);
                if(get_function->ftype==f_native){
                    vector arguments;
                    vector_init(&arguments);

                    // call get_function with o and key as arguments
                    vector_add(&arguments, o);
                    string* key_string=new_string();
                    key_string->value=strdup(key);
                    vector_add(&arguments, (object*)key_string);

                    object* result=get_function->pointer(arguments);
                    delete_unreferenced((object*)key_string);
                    vector_free(&arguments);
                    return result;
                } else {
                    ERROR(NOT_IMPLEMENTED, "Only native functions can be used as get override for now.");
                    return (object*)new_null();
                }
            }

            object** map_get_result=map_get(&((struct table*)o)->fields, key);
            if(map_get_result==NULL){// there's no object at this key
                return (object*)new_null();
            }else {
                return *map_get_result;
            }
        }
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Can't index object of type <%s>", OBJECT_TYPE_NAMES[o->type]);
            return (object*)new_null();
    }
}

void set(object* o, char*key, object* value){
    CHECK_OBJECT(o);
    switch(o->type){
        case t_table:
        {
            object** value_at_key = map_get(&((struct table*)o)->fields, key);
            if(value_at_key!=NULL){
                (*value_at_key)->ref_count--;// table no longer holds a reference to this object
                delete_unreferenced(*value_at_key);
                if(value->type==t_null){
                    map_remove(&((struct table*)o)->fields, key);// setting key to null removes it
                    return;
                }
            }
            value->ref_count++;// now the value is referenced by table
            map_set(&((struct table*)o)->fields, key, value);// key is empty so it only needs to be set to point to value
            break;
        }
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Can't index object of type <%s>", OBJECT_TYPE_NAMES[o->type]);
    }
}

object* call(object* o, vector arguments){
    switch(o->type){
        case t_function:
        {
            return call_function((function*)o, arguments);
        }
        case t_table:
        {
            object* call_field=get(o, "call");
            return call(call_field, arguments);
        }
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Can't call object of type <%s>", OBJECT_TYPE_NAMES[o->type]);
            return (object*)new_null();
    }
}