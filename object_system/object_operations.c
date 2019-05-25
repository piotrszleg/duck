#include "object_operations.h"

#define INITIAL_BUFFER_SIZE 16

object patching_table={t_table};

bool is_number(const char *s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return false;
    }
    return true;
}

// TODO:  write tests
bool is_falsy(object o){
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
            return o.tp->array_size==0 && o.tp->map_size==0;// empty table is falsy
        default:
            THROW_ERROR(INCORRECT_OBJECT_POINTER, "Incorrect object pointer passed to is_falsy function.");
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
            number_init(&result);
            if(o.type==t_null){
                result.value=0;// null is zero
                return result;
            } else if(o.type==t_string && is_number(o.text)){
                result.value=atoi(o.text);// convert string to int if it contains number
                return result;
            }
            // intentional fallthrough
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
        STRING_OBJECT(call_string, "call");
        object call_field=get(o, call_string);
        return find_call_function(call_field);
    } else {
        object n={t_null};
        return n;
    }
}

object find_function(object o, const char* function_name){
    STRING_OBJECT(function_name_string, function_name);
    return find_call_function(get(o, function_name_string));
}

object monkey_patching(const char* function_name, object* arguments, int arguments_count){
    // attempt to get object from field named after a's type in operations_table
    object type_table=get(patching_table, to_string(OBJECT_TYPE_NAMES[arguments[0].type]));
    if(type_table.type!=t_null){
        object operator_function=find_function(type_table, function_name);
        if(operator_function.type!=t_null){
            // call get_function a and b as arguments
            object result=call(operator_function, arguments, arguments_count);
            return result;
        }
    }
    return null_const;
}

#define MONKEY_PATCH(function_name, arguments, arguments_count) \
    object patching_result=monkey_patching(function_name, arguments, arguments_count); \
    if(patching_result.type!=t_null) return patching_result;

int sign(int x){
    return (x > 0) - (x < 0);
}

#define COMPARISION_ERROR 2
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
    if(a.type!=b.type){
        return COMPARISION_ERROR;
    }
    switch(a.type){
        case t_string:
            return strcmp(a.text, b.text);
        case t_number:
            return sign(a.value-b.value);
        // avoid comparing tables for now
        default:
            return COMPARISION_ERROR;
    }
}

object operator(object a, object b, const char* op){
    MONKEY_PATCH(op, ((object[]){a, b}), 2);
    if(a.type==t_table){
        object operator_function=find_function(a, op);
        if(operator_function.type!=t_null){
            // call get_function a and b as arguments
            object result=call(operator_function, ((object[]){a, b}), 2);
            return result;
        }
    }
    #define OP_CASE(operator_name) if(strcmp(op, operator_name)==0)
    OP_CASE("=="){
        int comparision_result=compare(a, b);
        if(comparision_result!=COMPARISION_ERROR)
            return create_number(comparision_result==0);
    }
    OP_CASE("!="){
        int comparision_result=compare(a, b);
        if(comparision_result!=COMPARISION_ERROR)
            return create_number(!comparision_result!=0);
    }
    OP_CASE(">"){
        int comparision_result=compare(a, b);
        if(comparision_result!=COMPARISION_ERROR)
            return create_number(comparision_result==1);
    }
    OP_CASE("<"){
        int comparison_result=compare(a, b);
        if(comparison_result!=COMPARISION_ERROR)
            return create_number(comparison_result==-1);
    }
    OP_CASE(">="){
        int comparison_result=compare(a, b);
        if(comparison_result!=COMPARISION_ERROR)
            return create_number(comparison_result==1||comparison_result==0);
    }
    OP_CASE("<="){
        int comparison_result=compare(a, b);
        if(comparison_result!=COMPARISION_ERROR)
            return create_number(comparison_result==-1||comparison_result==0);
    }
    OP_CASE("||"){
        if(!is_falsy(a)){
            return a;
        } else if (!is_falsy(b)){
            return b;
        } else {
            object result={t_null};
            return result;
        }
    }
    OP_CASE("&&"){
        if(is_falsy(a)){
            return a;
        } else if (is_falsy(b)){
            return b;
        } else{
            return b;
        }
    }
    OP_CASE("!"){
        return create_number(is_falsy(a));
    }
    OP_CASE("-"){
        if(a.type==t_number && b.type==t_null){
            a.value=-a.value;
            return a;
        }
    }
    OP_CASE(">>"){
        return new_pipe(a, b);
    }
    OP_CASE("<<"){
        return new_binding(a, b);
    }
    OP_CASE("##"){
        if(a.type==t_table){
            table_iterator it=start_iteration(a.tp);
            for(iteration_result i=table_next(&it); !i.finished; i=table_next(&it)) {
                call(b, (object[]){i.key, i.value}, 2);
            }
            return null_const;
        }
    }
    OP_CASE("#"){
        if(a.type==t_table){
            table_iterator it=start_iteration(a.tp);
            for(iteration_result i=table_next(&it); !i.finished; i=table_next(&it)) {
                call(b, (object[]){i.value}, 1);
            }
            return null_const;
        }
    }
    
    if(a.type==t_string){
        if(a.type!=b.type){
            b=cast(b, a.type);
        }
        OP_CASE("+"){
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
        OP_CASE("+"){
            return create_number(a.value+b.value);
        }
        OP_CASE("-"){
            return create_number(a.value-b.value);
        }
        OP_CASE("*"){
            return create_number(a.value*b.value);
        }
        OP_CASE("/"){
            return create_number(a.value/b.value);
        }
    }
    object causes[]={a, b};
    RETURN_ERROR("OperatorError", multiple_causes(causes, 2), "Can't perform operotion '%s' on objects of type <%s> and <%s>", op, OBJECT_TYPE_NAMES[a.type], OBJECT_TYPE_NAMES[b.type]);
}

char* stringify(object o){
    object patching_result=monkey_patching("stringify", &o, 1);
    if(patching_result.type!=t_null){
        return stringify_object(patching_result);
    }
    if(o.type==t_table){
        object stringify_override=find_function(o, "stringify");
        if(stringify_override.type!=t_null){
            object result=call(stringify_override, &o, 1);
            return stringify_object(result);
        }
    }
    return stringify_object(o);
}

char* suprintf (const char * format, ...)
{
    char* buffer=malloc(INITIAL_BUFFER_SIZE*sizeof(char));
    CHECK_ALLOCATION(buffer);
    int buffer_size=INITIAL_BUFFER_SIZE;
    va_list args;
    va_start (args, format);
    while(vsnprintf (buffer, buffer_size, format, args)>=buffer_size){
        buffer_size*=2;
        buffer=realloc(buffer, buffer_size*sizeof(char));
        CHECK_ALLOCATION(buffer);
    }
    va_end (args);
    return buffer;
}

char* stringify_object(object o){
    switch(o.type){
        case t_string:
        {
            /*int length=strlen(o.text+3);
            char* buffer=malloc(length*sizeof(char));
            snprintf(buffer, length, "\"%s\"", o.text);
            return buffer;*/
            return strdup(o.text);
        }
        case t_number:
        {
            int ceiled=o.value;
            if(((float)ceiled)==o.value){
                return suprintf("%d", ceiled);
            } else {
                return suprintf("%f", o.value);
            }
        }
        case t_table:
            return stringify_table(o.tp);
        case t_function:
        {
            function* f=o.fp;
            if(f->argument_names!=NULL){
                char* buffer=malloc(STRINGIFY_BUFFER_SIZE*sizeof(char));
                CHECK_ALLOCATION(buffer);
                buffer[0]='\0';
                int buffer_size=STRINGIFY_BUFFER_SIZE;
                int buffer_filled=0;// how many characters were written to the buffer
                
                // if buffer isn't big enough to hold the added string characters double its size
                #define BUFFER_WRITE(string, count) \
                    while(buffer_size<=buffer_filled+count){ \
                        buffer_size*=2; \
                        buffer=realloc(buffer, buffer_size*sizeof(char)); \
                    } \
                    strncat(buffer, string, buffer_size); \
                    buffer_filled+=count;
                
                BUFFER_WRITE("function(", 9);
                int first=1;
                for (int i = 0; i < f->arguments_count; i++){
                    char* argument_name=f->argument_names[i];
                    int character_count=strlen(argument_name);

                    if(first){
                        BUFFER_WRITE(argument_name, character_count);
                        first=0;
                    } else {
                        int formatted_count=3+character_count;
                        char* argument_buffer=malloc(formatted_count*sizeof(char));
                        snprintf(argument_buffer, formatted_count, ", %s", argument_name);
                        BUFFER_WRITE(argument_buffer, formatted_count);
                        free(argument_buffer);
                    }
                }
                BUFFER_WRITE(")", 1);
                
                buffer[buffer_size-1]='\0';// to make sure that the string won't overflow

                char* buffer_truncated=strdup(buffer);
                free(buffer);
                return buffer_truncated;
            } else if(f->arguments_count>0){
                char* buffer=malloc(16*sizeof(char*));
                snprintf(buffer, 16, "function(%i)", f->arguments_count);
                return buffer;
            } else {
                return strdup("function()");
            }
        }
        case t_pointer:
            return strdup("<pointer>");
        case t_null:
            return strdup("<null>");
        default:
            return strdup("<INCORRECT_OBJECT_POINTER>");
    }
}

object get(object o, object key){
    if(o.tp!=patching_table.tp) {// avoid cycling call to get in patching table
        MONKEY_PATCH("get", ((object[]){o, key}), 2);
    }
    if(o.type==t_table){
        // try to get "get" operator overriding function from the table and use it
        object map_get_override=get_table(o.tp, to_string("get"));
        if(map_get_override.type!=t_null){
            object arguments[]={o, key};

            object result=call(map_get_override, arguments, 2);
            return result;
        } else {
            // simply get key from table's map
            return get_table(o.tp, key);
        }
    } else if(o.type==t_string){
        object number_key=cast(key, t_number);
        if(number_key.type==t_number){
            if(number_key.value<strlen(o.text) && number_key.value>=0){
                char* character_string=malloc(2*sizeof(char));
                character_string[0]=o.text[(int)number_key.value];
                character_string[1]='\0';
                return to_string(character_string);
            } else {
                RETURN_ERROR("WRONG_ARGUMENT_TYPE", multiple_causes((object[]){o, key}, 2), 
                "Index %i is out of bounds of string \"%s\"", (int)number_key.value, o.text);
            }
        } else {
            return number_key;// casting failed, return error object
        }
    } else {
        RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Can't index object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
    }
}

object set(object o, object key, object value){
    MONKEY_PATCH("set", ((object[]){o, key, value}), 3);
    if(o.type==t_table){
        // try to get "get" operator overriding function from the table and use it
        object set_override=get_table(o.tp, to_string("set"));
        if(set_override.type!=t_null){
            return call(set_override, (object[]){o, key, value}, 3);
        } else {
            set_table(o.tp, key, value);
            return value;
        }
    } else {
        RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Can't index object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
    }
}

object* concat_arguments(object head, object* tail, int tail_count){
    object* result=malloc(sizeof(object)*(tail_count+1));
    result[0]=head;
    for(int i=0; i<tail_count; i++){
        result[i+1]=tail[i];
    }
    return result;
}

object call(object o, object* arguments, int arguments_count) {
    object* arguments_with_self=concat_arguments(o, arguments, arguments_count);
    object patching_result=monkey_patching("call", arguments_with_self, arguments_count+1);
    if(patching_result.type!=t_null){
        free(arguments_with_self);
        return patching_result;
    }
    switch(o.type){
        case t_function:
        {
            return call_function(o.fp, arguments, arguments_count);
        }
        case t_table:
        {
            object call_field=find_function(o, "call");
            if(call_field.type!=t_null){
                // add o object as a first argument
                object result=call(call_field, arguments_with_self, arguments_count+1);
                free(arguments_with_self);
                return result;
            }// else go to default label
        }
        default:
            RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Can't call object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
    }
}

void call_destroy(object o){
    object destroy_override=find_function(o, "destroy");
    if(destroy_override.type!=t_null){
        call(destroy_override, &o, 1);
    }
}