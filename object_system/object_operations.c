#include "object_operations.h"

#define INITIAL_BUFFER_SIZE 8
#define STRINGIFY_BUFFER_SIZE 200

int is_number(const char *s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return 0;
    }
    return 1;
}

// TODO:  write tests
int is_falsy(object o){
    switch(o.type){
        case t_null:
            return 1;// null is falsy
        case t_function:
            return 0;// every function isn't falsy
        case t_string:
            return strlen(o.text)==0;// "" (string of length 0) is falsy
        case t_number:
            return o.value==0;// 0 is falsy
        case t_table:
            return o.tp->fields.base.nnodes==0;// empty table is falsy
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Incorrect object pointer passed to is_falsy function.");
    }
}

int sign(int x){
    return (x > 0) - (x < 0);
}

// if a>b returns 1 if a<b returns -1, if a==b returns 0
int compare(object a, object b){
    // null is always less than anything
    if(a.type!=t_null && b.type==t_null){
        return 1;
    }
    if(b.type!=t_null && a.type==t_null){
        return -1;
    }
    if(b.type==t_null && a.type==t_null){
        return 0;
    }
    switch(a.type){
        case t_string:
            if(a.type==b.type){
                return strcmp(a.text, b.text);
            }
        case t_number:
            if(a.type==b.type){
                return sign(a.value-b.value);
            }
        case t_table:
            if(a.type==b.type){
                return sign(a.tp->fields.base.nnodes-b.tp->fields.base.nnodes);
            }
        default:
            ERROR(WRONG_ARGUMENT_TYPE, "Can't compare <%s> to <%s>", OBJECT_TYPE_NAMES[a.type], OBJECT_TYPE_NAMES[b.type]);
    }
}

object cast(object o, object_type type){
    if(o.type==type){
        return o;
    }
    switch(type){
        case t_string:
        {
            object result;
            string_init(&result);
            result.text=stringify(o);
            return result;
        }
        case t_number:
        {
            object result;
            string_init(&result);
            if(o.type==t_null){
                result.value=0;// null is zero
                return result;
            } else if(o.type==t_string && is_number(o.text)){
                result.value=atoi(o.text);// convert string to int if it contains number
                return result;
            }
        }
        case t_function:
        {
            object called=o;
            do{
                // recursively search for object of type function under key "call" inside of 'o' object structure
                called=get(called, "call");
                // get will fail at some point so it shouldn't create an infinite loop
            } while(called.type!=t_function);
            return called;
        }
        default:
            RETURN_ERROR("TYPE_CONVERSION_FAILURE", o, "Can't convert from <%s> to <%s>", OBJECT_TYPE_NAMES[o.type], OBJECT_TYPE_NAMES[type]);
    }
}

object create_number(int value){
    object n;
    number_init(&n);
    n.value=value;
    return n;
}

object find_call_function(object o){
    if(o.type==t_function){
        return o;
    } else if(o.type==t_table){
        object call_field=get(o, "call");
        return find_call_function(call_field);
    } else {
        object n={t_null};
        return n;
    }
}

object find_function(object o, char* function_name){
    return find_call_function(get(o, function_name));
}

object operator(object a, object b, char* op){
    if(a.type==t_table){
        object operator_function=find_function(a, op);
        if(operator_function.type!=t_null){
            // call get_function a and b as arguments
            object arguments[]={a, b};
            object result=call(operator_function, arguments, 2);
            return result;
        }
    }
    if(strcmp(op, "==")==0){
        return create_number(compare(a, b)==0);
    }
    if(strcmp(op, "!=")==0){
        return create_number(compare(a, b)!=0);
    }
    if(strcmp(op, ">")==0){
        return create_number(compare(a, b)==1);
    }
    if(strcmp(op, "<")==0){
        return create_number(compare(a, b)==-1);
    }
    if(strcmp(op, ">=")==0){
        int comparison_result=compare(a, b);
        return create_number(comparison_result==1||comparison_result==0);
    }
    if(strcmp(op, "<=")==0){
        int comparison_result=compare(a, b);
        return create_number(comparison_result==-1||comparison_result==0);
    }
    if(strcmp(op, "||")==0){
        if(!is_falsy(a)){
            return a;
        } else if (!is_falsy(b)){
            return b;
        } else {
            object result={t_null};
            return result;
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
        return create_number(is_falsy(a));
    }
    if(strcmp(op, "-")==0 && a.type==t_number && b.type==t_null){
        a.value=-a.value;
        return a;
    }
    
    if(a.type==t_string){
        if(a.type!=b.type){
            b=cast(b, a.type);
        }
        if(strcmp(op, "+")==0){
            char* buffer=malloc(sizeof(char)*1024);
            CHECK_ALLOCATION(buffer);
            strcpy(buffer, a.text);
            strcat(buffer, b.text);
            object result;
            string_init(&result);
            result.text=buffer;
            return result;
        }
    } else if(a.type==t_number){
        if(a.type!=b.type){
                b=cast(b, a.type);
        }
        object result;
        number_init(&result);
        if(strcmp(op, "+")==0){
            return create_number(a.value+b.value);
        }
        if(strcmp(op, "-")==0){
            return create_number(a.value-b.value);
        }
        if(strcmp(op, "*")==0){
            return create_number(a.value*b.value);
        }
        if(strcmp(op, "/")==0){
            return create_number(a.value/b.value);
        }
    }
    RETURN_ERROR("WRONG_ARGUMENT_TYPE", a, "Can't perform operotion '%s' object of type <%s> and <%s>", op, OBJECT_TYPE_NAMES[a.type], OBJECT_TYPE_NAMES[b.type]);
}

char* stringify(object o){
    if(o.type==t_table){
        object stringify_override=find_function(o, "stringify");
        if(stringify_override.type!=t_null){
            object arguments[]={o};
            object result=call(stringify_override, arguments, 1);
            return stringify_object(result);
        }
    }
    return stringify_object(o);
}

#define CALL_SNPRINTF(buffer, format, ...) \
    buffer=malloc(INITIAL_BUFFER_SIZE*sizeof(char)); \
    int buffer_size=INITIAL_BUFFER_SIZE; \
    CHECK_ALLOCATION(buffer); \
    while(1) { /* repeatedly call snprintf until buffer is big enough to hold formatted string */ \
        int snprintf_result=snprintf(buffer, buffer_size, format, __VA_ARGS__); \
        if(snprintf_result<buffer_size){ \
            break; /*success*/ \
        } \
        if(snprintf_result<0){ \
            free(buffer); \
            ERROR(STRINGIFICATION_ERROR, "Error while calling snprintf(\"%s\")", format); \
            buffer=strdup("<ERROR>"); \
            break; \
        } \
        /*double buffer size*/ \
        buffer_size*=2; \
        buffer=realloc(buffer, buffer_size*sizeof(char)); \
        CHECK_ALLOCATION(buffer); \
    }

char* stringify_object(object o){
    switch(o.type){
        case t_string:
            return strdup(o.text);
        case t_number:
        {
            char* buffer;
            int ceiled=o.value;
            if(((float)ceiled)==o.value){
                CALL_SNPRINTF(buffer, "%d", ceiled);
            } else {
                CALL_SNPRINTF(buffer, "%f", o.value);
            }
            return buffer;
        }
        case t_table:
            {
                table_* t=o.tp;
                char* buffer=malloc(STRINGIFY_BUFFER_SIZE*sizeof(char));
                CHECK_ALLOCATION(buffer);
                int buffer_size=STRINGIFY_BUFFER_SIZE;
                buffer[0]='[';
                buffer[1]='\0';
                int buffer_filled=1;// how many characters were written to the buffer
                const char *key;
               
                int first=1;
                map_iter_t iter = map_iter(&m);
                while ((key = map_next(&t->fields, &iter))) {
                    object value=*map_get(&t->fields, key);
                    int self_reference=value.tp==t;
                    char* value_stringified= stringify(value);

                    int formatted_count=strlen(key)+1+strlen(value_stringified)+1;
                    if(!first){ 
                        formatted_count+=2;
                    }
                    char* pair_buffer=malloc(formatted_count*sizeof(char));
                    if(first){
                        snprintf(pair_buffer, formatted_count, "%s=%s", key, value_stringified);
                    } else {
                        snprintf(pair_buffer, formatted_count, ", %s=%s", key, value_stringified);
                    }
                    // if buffer isn't big enough to hold the pair double it
                    while(buffer_size<=(buffer_filled+formatted_count)*sizeof(char)){
                        buffer_size*=2;
                        buffer=realloc(buffer, buffer_size*sizeof(char));
                    }
                    strncat(buffer, pair_buffer, buffer_size);
                    buffer_filled+=formatted_count;
                    first=0;
                }
                // check if buffer can hold additional ']' character
                if(buffer_size<=buffer_filled+1){
                    buffer_size*=2;
                    buffer=realloc(buffer, buffer_size*sizeof(char));
                }
                strncat(buffer, "]", buffer_size);
                buffer[buffer_size-1]='\0';// to make sure that the string won't overflow

                char* buffer_truncated=strdup(buffer);
                free(buffer);
                return buffer_truncated;
            }
        case t_function:
            return strdup("<function>");
        case t_null:
            return strdup("<null>");
        default:
            return strdup("<INCORRECT_OBJECT_POINTER>");
    }
}

object get_table(table_* t, char* key){
    object* map_get_result=map_get(&t->fields, key);
    if(map_get_result==NULL){// there's no object at this key
        return null_const;
    }else {
        return *map_get_result;
    }
}

object get(object o, char* key){
    if(o.type==t_table){
        // try to get "get" operator overriding function from the table and use it
        object* map_get_override=map_get(&o.tp->fields, "get");
        if(map_get_override!=NULL && map_get_override->type==t_function){

            // create object to hold key value
            object key_string;
            string_init(&key_string);
            key_string.text=strdup(key);

            object arguments[]={o, key_string};

            object result=call_function(map_get_override->fp, arguments, 2);
            object_deinit(&key_string);
            return result;
        } else {
            // simply get key from table's map
            return get_table(o.tp, key);
        }
    } else {
        RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Can't index object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
    }
}

void set_table(table_* t, char* key, object value){
    object_map_t* fields=&t->fields;
    object* value_at_key = map_get(fields, key);
    if(value_at_key!=NULL){
        dereference(value_at_key);// table no longer holds a reference to this object
        delete_unreferenced(value_at_key);
        if(value.type==t_null){
            map_remove(fields, key);// setting key to null removes it
            return;
        }
    }
    reference(&value);// now the value is referenced by table
    map_set(fields, key, value);// key is empty so it only needs to be set to point to value
}

void set(object o, char* key, object value){
    if(o.type==t_table){
        // try to get "get" operator overriding function from the table and use it
        object set_override=find_function(o, "stringify");
        if(set_override.type!=t_null){
            // create object to hold key value
            object key_string;
            string_init(&key_string);
            key_string.text=strdup(key);

            object arguments[]={o, key_string, value};
            call(set_override, arguments, 3);
            object_deinit(&key_string);
        } else {
            set_table(o.tp, key, value);
        }
    } else {
        ERROR(WRONG_ARGUMENT_TYPE, "Can't index object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
    }
}

object call(object o, object* arguments, int arguments_count) {
    switch(o.type){
        case t_function:
        {
            return call_function(o.fp, arguments, arguments_count);
        }
        case t_table:
        {
            object call_field=find_function(o, "call");
            if(call_field.type!=t_null){
                return call(call_field, arguments, arguments_count);
            }// else go to default label
        }
        default:
            RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Can't call object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
    }
}

object new_error(char* type, object cause, char* message){
    object err;
    table_init(&err);

    object err_type;
    string_init(&err_type);
    err_type.text=type;
    set(err, "type", err_type);

    object err_message;
    string_init(&err_message);
    err_message.text=message;
    set(err, "message", err_message);

    object one;
    number_init(&one);
    one.value=1;
    set(err, "is_error", one);

    set(err, "cause", cause);

    return err;
}