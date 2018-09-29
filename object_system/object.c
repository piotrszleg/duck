#include "object.h"

const char* OBJECT_TYPE_NAMES[]={
    "null",
    "number",
    "function",
    "string",
    "table"
};

// allocates object of type t in pointer variable named result
#define ALLOC_OBJECT(t, t_enum) \
    object* result= malloc(sizeof(t)); \
    if(!result) { ERROR(MEMORY_ALLOCATION_FAILURE , "Memory allocation failure"); } \
    result->type=t_enum;

// all objects should be made and removed using these functions

object* object_new(object_type type){
    switch(type){
        case t_string:
            {
                ALLOC_OBJECT(string, type);
                return result;
            }

        case t_number:
            {
                ALLOC_OBJECT(number, type);
                return result;
            }

        case t_function:
            {
                ALLOC_OBJECT(function, type);
                return result;
            }

        case t_table:
            {
                ALLOC_OBJECT(table, type);
                map_init(&((table*)result)->fields);
                return result;
            }
        case t_null:
            {
                ALLOC_OBJECT(object, t_null);
                return result;
            }
        default:
            return NULL;
    }
}

void object_delete(object* o){
    switch(o->type){
        case t_string:
            free((string*)o);
            break;
        case t_number:
            free((number*)o);
            break;
        case t_function:
            free((table*)o);
            break;
        case t_table:
            map_deinit(&((table*)o)->fields);
            free((table*)o);
            break;
        default:
            free(o);
    }
    o=NULL;
}

int is_number(const char *s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return 0;
    }
    return 1;
}

object* cast(object* o, object_type type){
    switch(type){
        case t_string:
            {
                char* buffer=malloc(sizeof(char)*1024);
                strcpy(buffer, stringify(o));
                object* result=object_new(t_string);
                ((string*)result)->value=buffer;
                return result;
            }
        case t_number:
            {
                if(o->type==t_number){
                    return o;
                } else {
                    object* result=object_new(t_number);
                    if(o->type==t_null){
                        ((number*)result)->value=0;// null is the only false'y value for now
                    } else if(o->type==t_string && is_number(((string*)o)->value)){
                        ((number*)result)->value=atoi(((string*)o)->value);// convert string to int if it contains number
                    } else {
                        ((number*)result)->value=1;// value is not false'y
                    }
                    return result;
                }
            }
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Can't convert from <%s> to <%s>", OBJECT_TYPE_NAMES[o->type], OBJECT_TYPE_NAMES[type]);
            return object_new(t_null);// conversion can't be performed
    }
}

object* add(object* a, object* b){
    if(a->type!=b->type){
        b=cast(b, a->type);
    }
    switch(a->type){
        case t_string:
            {
                char* buffer=malloc(sizeof(char)*1024);
                if(!buffer){ ERROR(MEMORY_ALLOCATION_FAILURE, "Memory allocation failure"); }
                strcpy(buffer, ((string*) a)->value);
                strcat(buffer, ((string*) b)->value);
                object* result=object_new(t_string);
                ((string*)result)->value=buffer;

                return result;
            }

        case t_number:
            {
                object* result=object_new(t_number);
                ((number*)result)->value=((number*) a)->value+ ((number*) b)->value;
                return result;
            }
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Can't add object of type <%s> and <%s>", OBJECT_TYPE_NAMES[a->type], OBJECT_TYPE_NAMES[b->type]);
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
            ERROR(INCORRECT_OBJECT_POINTER, "Object at %#8x has no valid type value", o);
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
                return object_new(t_null);
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
