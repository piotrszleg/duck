#include "object_operations.h"

#define INITIAL_BUFFER_SIZE 16

bool is_number(const char *s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return false;
    }
    return true;
}

// TODO:  write tests
bool is_falsy(Object o){
    switch(o.type){
        case t_null:
            return 1;// null is falsy
        case t_string:
            return strlen(o.text)==0;// "" (string of length 0) is falsy
        case t_int:
            return o.int_value==0;// 0 is falsy
        case t_float:
            return o.float_value==0;// 0 is falsy
        case t_table:
            return o.tp->array_size==0 && o.tp->map_size==0;// empty Table is falsy
        case t_function:
        case t_gc_pointer:
        case t_coroutine:
            return 0;
        default:
            THROW_ERROR(INCORRECT_OBJECT_POINTER, "Incorrect object pointer passed to is_falsy function.");
    }
}

Object cast(Executor* E, Object o, ObjectType type){
    if(o.type==type){
        return o;
    }
    switch(type){
        case t_string:
        {
            Object result;
            string_init(&result);
            result.text=stringify(E, o);
            return result;
        }
        case t_int:
        {
            Object result;
            float_init(&result);
            if(o.type==t_null){
                result.int_value=0;// null is zero
                return result;
            } else if(o.type==t_string && is_number(o.text)){
                result.int_value=strtol(o.text, NULL, 10);// convert string to int if it contains number
                return result;
            }
            break;
        }
        case t_float:
        {
            Object result;
            float_init(&result);
            if(o.type==t_null){
                result.float_value=0;// null is zero
                return result;
            } else if(o.type==t_string && is_number(o.text)){
                result.float_value=strtof(o.text, NULL);// convert string to int if it contains number
                return result;
            }
            break;
        }
        default:;
    }
    RETURN_ERROR("TYPE_CONVERSION_FAILURE", o, "Can't convert from <%s> to <%s>", OBJECT_TYPE_NAMES[o.type], OBJECT_TYPE_NAMES[type]);
}

Object find_call_function(Executor* E, Object o){
    if(o.type==t_function){
        return o;
    } else if(o.type==t_table){
        STRING_OBJECT(call_string, "call");
        Object call_field=get(E, o, call_string);
        return find_call_function(E, call_field);
    } else {
        Object n={t_null};
        return n;
    }
}

Object find_function(Executor* E, Object o, const char* function_name){
    STRING_OBJECT(function_name_string, function_name);
    return find_call_function(E, get(E, o, function_name_string));
}

int sign(int x){
    return (x > 0) - (x < 0);
}

#define COMPARISION_ERROR 2
// if a>b returns 1 if a<b returns -1, if a==b returns 0
int compare(Object a, Object b){
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
        case t_int:
            return sign(a.int_value-b.int_value);
        case t_float:
            return sign(a.float_value-b.float_value);
        // avoid comparing tables for now
        default:
            return COMPARISION_ERROR;
    }
}

Object coroutine_iterator_next(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object coroutine=get(E, self, to_string("coroutine"));
    Object value=call(E, coroutine, NULL, 0);
    Object result;
    table_init(E, &result);
    set(E, result, to_string("value"), value);

    REQUIRE_TYPE(coroutine, t_coroutine);
    set(E, result, to_string("finished"), to_int(coroutine.co->state==co_finished));

    return result;
}

Object coroutine_iterator(Executor* E, Object coroutine){
    Object iterator;
    table_init(E, &iterator);
    set(E, iterator, to_string("coroutine"), coroutine);
    set_function(E, iterator, "call", 1, false, coroutine_iterator_next);
    return iterator;
}

Object get_iterator(Executor* E, Object o){
    if(o.type==t_table){
        Object iterator_override=find_function(E, o, "iterator");
        if(iterator_override.type!=t_null){
            return call(E, iterator_override, &o, 1);
        } else {
            return table_get_iterator_object(E, &o, 1);
        }
    } else if(o.type==t_coroutine){
        return coroutine_iterator(E, o);
    } else {
        RETURN_ERROR("OperatorError", o, "Can't get operator of object of type %s.", OBJECT_TYPE_NAMES[o.type]);
    }
}

Object operator(Executor* E, Object a, Object b, const char* op){
    if(a.type==t_table){
        Object operator_function=find_function(E, a, op);
        if(operator_function.type!=t_null){
            // call get_function a and b as arguments
            Object result=call(E, operator_function, ((Object[]){a, b}), 2);
            return result;
        }
    }
    #define OP_CASE(operator_name) if(strcmp(op, operator_name)==0)
    OP_CASE("=="){
        int comparision_result=compare(a, b);
        if(comparision_result!=COMPARISION_ERROR)
            return to_int(comparision_result==0);
    }
    OP_CASE("!="){
        int comparision_result=compare(a, b);
        if(comparision_result!=COMPARISION_ERROR)
            return to_int(comparision_result!=0);
    }
    OP_CASE(">"){
        int comparision_result=compare(a, b);
        if(comparision_result!=COMPARISION_ERROR)
            return to_int(comparision_result==1);
    }
    OP_CASE("<"){
        int comparison_result=compare(a, b);
        if(comparison_result!=COMPARISION_ERROR)
            return to_int(comparison_result==-1);
    }
    OP_CASE(">="){
        int comparison_result=compare(a, b);
        if(comparison_result!=COMPARISION_ERROR)
            return to_int(comparison_result==1||comparison_result==0);
    }
    OP_CASE("<="){
        int comparison_result=compare(a, b);
        if(comparison_result!=COMPARISION_ERROR)
            return to_int(comparison_result==-1||comparison_result==0);
    }
    OP_CASE("||"){
        if(!is_falsy(a)){
            return a;
        } else if (!is_falsy(b)){
            return b;
        } else {
            Object result={t_null};
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
        return to_int(is_falsy(b));
    }
    OP_CASE("-"){
        if(a.type==t_null && b.type==t_int){
            return to_int(-b.int_value);
        } else if(a.type==t_null && b.type==t_float){
            return to_int(-b.float_value);
        }
    }
    OP_CASE(">>"){
        return new_pipe(E, a, b);
    }
    OP_CASE("<<"){
        return new_binding(E, a, b);
    }
    // call b with arguments key and value for each iteration
    OP_CASE("##"){
        Object it;
        Object call_result=null_const;
        FOREACH(a, it, 
            call_result=call(E, b, (Object[]){get(E, it, to_string("key")), get(E, it, to_string("value"))}, 2);
        )
        return call_result;
    }
    // same as above except only the values are passed to the function
    OP_CASE("#"){
        Object it;
        Object call_result=null_const;
        FOREACH(a, it, 
            call_result=call(E, b, (Object[]){get(E, it, to_string("value"))}, 1);
        )
        return call_result;
    }
    #define OP_CASE_NUMBERS(operator_name, operator, a_type) \
        if(a.type==a_type && strcmp(op, operator_name)==0) { \
            Object casted_b=cast(E, b, a_type); \
            if(casted_b.type!=a_type){ \
                return casted_b;/* casted_b must be type conversion error */ \
            } \
            Object result=a. operator casted_b; \
            destroy_unreferenced(b_casted); \
            return result; \
        }
    if(a.type==t_string){
        if(a.type!=b.type){
            b=cast(E, b, t_string);
        }
        OP_CASE("+"){
            char* buffer=malloc(sizeof(char)*1024);
            CHECK_ALLOCATION(buffer);
            strcpy(buffer, a.text);
            strcat(buffer, b.text);
            Object result;
            string_init(&result);
            result.text=buffer;
            return result;
        }
    }
    if(a.type==t_int) {
        Object b_casted;
        if(b.type!=t_int){
            Object b_casted=cast(E, b, t_int);
            if(b_casted.type!=t_int){
                return b_casted;// b_casted is conversion error
            }
        } else {
            b_casted=b;
        }
        Object result;
        int_init(&result);
        OP_CASE("+"){
            return to_int(a.int_value+b_casted.int_value);
        }
        OP_CASE("-"){
            return to_int(a.int_value-b_casted.int_value);
        }
        OP_CASE("*"){
            return to_int(a.int_value*b_casted.int_value);
        }
        OP_CASE("//"){
            return to_int(a.int_value/b_casted.int_value);
        }
        OP_CASE("%"){
            return to_int(a.int_value%b_casted.int_value);
        }
        OP_CASE("/"){
            return to_float((float)a.int_value/b_casted.int_value);
        }
        destroy_unreferenced(E, &b_casted);
    } else if(a.type==t_float) {
        Object b_casted;
        if(b.type!=t_float){
            Object b_casted=cast(E, b, t_float);
            if(b_casted.type!=t_float){
                return b_casted;// b_casted is conversion error
            }
        } else {
            b_casted=b;
        }
        Object result;
        float_init(&result);
        OP_CASE("+"){
            return to_float(a.float_value+b_casted.float_value);
        }
        OP_CASE("-"){
            return to_float(a.float_value-b_casted.float_value);
        }
        OP_CASE("*"){
            return to_float(a.float_value*b_casted.float_value);
        }
        OP_CASE("/"){
            return to_float((float)a.int_value/b_casted.int_value);
        }
        destroy_unreferenced(E, &b_casted);
    }
    RETURN_ERROR("OperatorError", multiple_causes(E, (Object[]){a, b}, 2), "Can't perform operotion '%s' on objects of type <%s> and <%s>", op, OBJECT_TYPE_NAMES[a.type], OBJECT_TYPE_NAMES[b.type]);
}

char* stringify(Executor* E, Object o){
    if(o.type==t_table){
        Object stringify_override=find_function(E, o, "stringify");
        if(stringify_override.type!=t_null){
            Object result=call(E, stringify_override, &o, 1);
            if(result.type!=t_string){
                return stringify_object(E, result);
            } else {
                return result.text;
            }
        }
    }
    return stringify_object(E, o);
}

char* suprintf (const char * format, ...){
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

// source: https://stackoverflow.com/questions/779875/what-is-the-function-to-replace-string-in-c
char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

char* stringify_object(Executor* E, Object o){
    switch(o.type){
        case t_string:
        {
            // escape quotes
            char* replacement_result=str_replace(o.text, "\"", "\\\"");
            char* text;
            char* result;
            if(replacement_result!=NULL){
                text=replacement_result;
            } else {
                text=o.text;
            }
            if(is_valid_name(text)){
                int length=strlen(text)+2;
                result=malloc(length*sizeof(char));
                snprintf(result, length, "'%s", text);
            } else {
                int length=strlen(text)+3;
                result=malloc(length*sizeof(char));
                snprintf(result, length, "\"%s\"", text);
            }

            if(replacement_result!=NULL){
                free(replacement_result);
            }
            return result;
           // return strdup(o.text);
        }
        case t_int:
            return suprintf("%d", o.int_value);
        case t_float:
            return suprintf("%f", o.float_value);
        case t_table:
            return stringify_table(E, o.tp);
        case t_function:
        {
            Function* f=o.fp;
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
        case t_coroutine:
            return strdup("<coroutine>");
        case t_pointer:
            return strdup("<pointer>");
        case t_null:
            return strdup("null");
        default:
            return strdup("<INCORRECT_OBJECT_POINTER>");
    }
}

Object get(Executor* E, Object o, Object key){
    if(o.type==t_table){
        // try to get "get" operator overriding function from the Table and use it
        Object map_get_override=table_get(o.tp, to_string("get"));
        if(map_get_override.type!=t_null){
            Object arguments[]={o, key};

            Object result=call(E, map_get_override, arguments, 2);
            return result;
        } else {
            // simply get key from Table's map
            return table_get(o.tp, key);
        }
    } else if(o.type==t_string){
        if(key.type==t_int){
            if(key.int_value<strlen(o.text) && key.int_value>=0){
                char* character_string=malloc(2*sizeof(char));
                character_string[0]=o.text[(int)key.int_value];
                character_string[1]='\0';
                return to_string(character_string);
            } else {
                RETURN_ERROR("WRONG_ARGUMENT_TYPE", multiple_causes(E, (Object[]){o, key}, 2), 
                "Index %i is out of bounds of string \"%s\"", key.int_value, o.text);
            }
        } else {
            RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Strings can be only indexed with ints");
        }
    } else {
        RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Can't index object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
    }
}

Object set(Executor* E, Object o, Object key, Object value){
    if(o.type==t_table){
        // try to get "get" operator overriding function from the Table and use it
        Object set_override=table_get(o.tp, to_string("set"));
        if(set_override.type!=t_null){
            return call(E, set_override, (Object[]){o, key, value}, 3);
        } else {
            table_set(E, o.tp, key, value);
            return value;
        }
    } else {
        RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Can't index object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
    }
}

Object* concat_arguments(Object head, Object* tail, int tail_count){
    Object* result=malloc(sizeof(Object)*(tail_count+1));
    result[0]=head;
    for(int i=0; i<tail_count; i++){
        result[i+1]=tail[i];
    }
    return result;
}

Object call(Executor* E, Object o, Object* arguments, int arguments_count) {
    Object* arguments_with_self=concat_arguments(o, arguments, arguments_count);
    switch(o.type){
        case t_function:
        {
            return call_function(E, o.fp, arguments, arguments_count);
        }
        case t_coroutine:
        {
            return call_coroutine(E, o.co, arguments, arguments_count);
        }
        case t_table:
        {
            Object call_field=find_function(E, o, "call");
            if(call_field.type!=t_null){
                // add o object as a first argument
                Object result=call(E, call_field, arguments_with_self, arguments_count+1);
                free(arguments_with_self);
                return result;
            }// else go to default label
        }
        default:
            RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Can't call object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
    }
}

Object copy(Executor* E, Object o){
    switch(o.type){
        case t_string:
        {
            return to_string(strdup(o.text));
        }
        case t_table:
        {
            Object copy_override=find_function(E, o, "copy");
            if(copy_override.type!=t_null){
                return call(E, copy_override, &o, 1);
            } else {
                Object copied;
                table_init(E, &copied);
                TableIterator it=table_get_iterator(o.tp);

                for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
                    table_set(E, copied.tp, copy(E, i.key), copy(E, i.value));
                }
                return copied;
            }
        }
        default: 
            return o;
    }
}

void call_destroy(Executor* E, Object o){
    Object destroy_override=find_function(E, o, "destroy");
    if(destroy_override.type!=t_null){
        Object destroy_result=call(E, destroy_override, &o, 1);
        dereference(E, &destroy_result);
    }
}

#undef ALREADY_DESTROYED_CHECK