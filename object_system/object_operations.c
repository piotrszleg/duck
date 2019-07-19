#include "object_operations.h"

#define INITIAL_BUFFER_SIZE 16

bool is_number(const char *s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return false;
    }
    return true;
}

bool is_truthy(Object o){
    return !is_falsy(o);
}

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
            return o.tp->elements_count==0;// empty table is falsy
        case t_function:
        case t_gc_pointer:
        case t_coroutine:
        case t_pointer:
            return false;
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
            int_init(&result);
            if(o.type==t_float){
                result.int_value=o.float_value;
                return result;
            } else if(o.type==t_null){
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
            if(o.type==t_int){
                result.float_value=o.int_value;
                return result;
            }else if(o.type==t_null){
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
        Object call_override=get(E, o, call_string);
        return find_call_function(E, call_override);
    } else {
        Object n={t_null};
        return n;
    }
}

Object find_function(Executor* E, Object o, const char* function_name){
    STRING_OBJECT(function_name_string, function_name);
    return find_call_function(E, get(E, o, function_name_string));
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
    if(a.type==t_float && b.type==t_int){
        return sign(a.float_value-b.int_value);
    }
    if(a.type==t_int && b.type==t_float){
        return sign((float)a.int_value-b.float_value);
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
        case t_table:
            return table_compare(a.tp, b.tp);
        case t_pointer:
            return a.p==b.p;
        case t_gc_pointer:
            return a.gcp==b.gcp;
        case t_function:
            if(a.fp->ftype!=b.fp->ftype){
                return 1;
            }
            switch(a.fp->ftype){
                case f_ast:
                case f_bytecode:
                    return sign((int)a.fp->source_pointer-(int)b.fp->source_pointer);
                case f_special:
                    return sign((int)a.fp->special_index-(int)b.fp->special_index);
                case f_native:
                    return sign((int)a.fp->native_pointer-(int)b.fp->native_pointer);
            }
        default:
            return COMPARISION_ERROR;
    }
}

Object coroutine_iterator_next(Executor* E, Object* arguments, int arguments_count){
    Object iterator=arguments[0];
    Object coroutine=get(E, iterator, to_string("coroutine"));
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
    set_function(E, iterator, "next", 1, false, coroutine_iterator_next);
    return iterator;
}

Object string_iterator_next(Executor* E, Object* arguments, int arguments_count){
    Object iterator=arguments[0];
    Object iterated=get(E, iterator, to_string("iterated"));
    REQUIRE_TYPE(iterated, t_string)
    Object index=get(E, iterator, to_string("index"));
    REQUIRE_TYPE(index, t_int)
    Object length=get(E, iterator, to_string("length"));
    REQUIRE_TYPE(length, t_int)
    Object result;
    table_init(E, &result);
    if(index.int_value<length.int_value){
        set(E, result, to_string("key"), index);
        char character[2]={iterated.text[index.int_value], '\0'};
        set(E, result, to_string("value"), to_string(character));
        set(E, iterator, to_string("index"), to_int(index.int_value+1));
    } else {
        set(E, result, to_string("finished"), to_int(1));
    }
    return result;
}

Object string_iterator(Executor* E, Object str){
    Object iterator;
    table_init(E, &iterator);
    set(E, iterator, to_string("iterated"), str);
    set(E, iterator, to_string("index"), to_int(0));
    set(E, iterator, to_string("length"), to_int(strlen(str.text)));
    set_function(E, iterator, "next", 1, false, string_iterator_next);
    return iterator;
}

Object get_iterator(Executor* E, Object o){
    switch(o.type){
        case t_table: {
            Object iterator_override=find_function(E, o, "iterator");
            if(iterator_override.type!=t_null){
                return call(E, iterator_override, &o, 1);
            } else {
                return table_get_iterator_object(E, &o, 1);
            }
        }
        case t_coroutine:
            return coroutine_iterator(E, o);
        case t_string:
            return string_iterator(E, o);
        default:
            RETURN_ERROR("ITERATION_ERROR", o, "Can't get iterator of object of type %s.", OBJECT_TYPE_NAMES[o.type]);
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
        if(is_truthy(a)){
            return a;
        } else if (is_truthy(b)){
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
    OP_CASE("--"){
        return to_pipe(E, a, b);
    }
    OP_CASE("><"){
        return to_binding(E, a, b);
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
        OP_CASE("+"){
            Object result;
            if(b.type!=t_string) {
                USING_STRING(stringify(E, b),
                    result=to_string(string_add(a.text, str)))
            } else {
                result=to_string(string_add(a.text, b.text));
            }
            return result;
        }
        OP_CASE("*"){
            if(b.type==t_int && b.int_value>0){
                return to_string(string_repeat(a.text, b.int_value));
            }
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
    RETURN_ERROR("OPERATOR_ERROR", multiple_causes(E, (Object[]){a, b}, 2), "Can't perform operotion '%s' on objects of type <%s> and <%s>", op, OBJECT_TYPE_NAMES[a.type], OBJECT_TYPE_NAMES[b.type]);
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

char* stringify_object(Executor* E, Object o){
    switch(o.type){
        case t_string:
        {
            ReplacementPair replacements[]={
                CONSTANT_REPLACEMENT_PAIR("\"", "\\\""),
                CONSTANT_REPLACEMENT_PAIR("\t", "\\t"),
                CONSTANT_REPLACEMENT_PAIR("\n", "\\n"),
                CONSTANT_REPLACEMENT_PAIR("\\", "\\\\")
            };
            // escape quotes
            char* replacement_result=string_replace_multiple(o.text, replacements, 4);
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
            return suprintf("coroutine(%#x)", (unsigned)o.co);
        case t_pointer:
            return suprintf("pointer(%#x)", (unsigned)o.p);
        case t_gc_pointer:
            return suprintf("gc_pointer(%#x)", (unsigned)o.gcp);
        case t_null:
            return strdup("null");
        default:
            return strdup("<INCORRECT_OBJECT_POINTER>");
    }
}

Object get(Executor* E, Object o, Object key){
    switch(o.type){
        case t_table:
        {
            // try to get "get" field overriding function from the Table and use it
            Object map_get_override=table_get(o.tp, to_string("get"));
            if(map_get_override.type!=t_null){
                Object arguments[]={o, key};
                Object result=call(E, map_get_override, arguments, 2);
                return result;
            } else {
                // simply get key from Table's map
                return table_get(o.tp, key);
            }
        }
        #define FIELD(identifier) if(key.type==t_string && strcmp(key.text, identifier)==0)
        case t_string:
            FIELD("length"){
                return to_int(strlen(o.text));
            } else if(key.type==t_int){
                if(key.int_value<strlen(o.text) && key.int_value>=0){
                    char* character_string=malloc(2*sizeof(char));
                    character_string[0]=o.text[(int)key.int_value];
                    character_string[1]='\0';
                    return to_string(character_string);
                } else {
                    RETURN_ERROR("GET_ERROR", multiple_causes(E, (Object[]){o, key}, 2), 
                    "Index %i is out of bounds of string \"%s\"", key.int_value, o.text);
                }
            }
            break;
        case t_coroutine:
            FIELD("state"){
                switch(o.co->state){
                    case co_uninitialized: return to_string("uninitialized");
                    case co_running: return to_string("running");
                    case co_finished: return to_string("finished");
                }
            }
            FIELD("finished"){
                return to_int(o.co->state==co_finished);
            }
            break;
        case t_function:
            FIELD("variadic"){
                return to_int(o.fp->variadic);
            }
            FIELD("arguments_count"){
                return to_int(o.fp->arguments_count);
            }
            break;
        default:;
    }
    RETURN_ERROR("GET_ERROR",  multiple_causes(E, (Object[]){o, key}, 2), "Can't get field in object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
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
        RETURN_ERROR("SET", o, "Can't set field in object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
    }
}

Object call(Executor* E, Object o, Object* arguments, int arguments_count) {
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
            Object call_override=find_function(E, o, "call");
            if(call_override.type!=t_null){
                // add o object as a first argument
                Object* arguments_with_self=malloc(sizeof(Object)*(arguments_count+1));
                arguments_with_self[0]=o;
                memcpy(arguments_with_self+1, arguments, arguments_count*sizeof(Object));

                Object result=call(E, call_override, arguments_with_self, arguments_count+1);
                free(arguments_with_self);
                return result;
            }// else go to default label
        }
        default:
            RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Can't call object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
    }
}

Object message_object(Executor* E, Object messaged, const char* message_identifier, Object* arguments, int arguments_count){
    switch(messaged.type) {
        case t_table:
        {
            Object message_override=find_function(E, messaged, "message");
            if(message_override.type!=t_null){
                // call "message" field of messaged, passing messaged, message_identifier and rest of the arguments
                Object* override_arguments=malloc(sizeof(Object)*(arguments_count+2));
                override_arguments[0]=messaged;
                override_arguments[1]=to_string(message_identifier);
                memcpy(override_arguments+2, arguments, arguments_count*sizeof(Object));

                Object result=call(E, message_override, override_arguments, arguments_count+2);
                free(override_arguments);
                return result;
            } else {
                // call message_identifier field of messaged, passing messaged, and rest of the arguments
                Object function_field=get(E, messaged, to_string(message_identifier));
                Object* arguments_with_self=malloc(sizeof(Object)*(arguments_count+1));
                arguments_with_self[0]=messaged;
                memcpy(arguments_with_self+1, arguments, arguments_count*sizeof(Object));
                call(E, function_field, arguments_with_self, arguments_count+1);
            }
            break;
        }
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

bool is_error(Executor* E, Object o){
    if(o.type!=t_table){
        return false;
    } else {
        Object is_error_object=get(E, o, to_string("error"));
        bool result=is_truthy(is_error_object);
        destroy_unreferenced(E, &is_error_object);
        return result;
    }
}

void call_destroy(Executor* E, Object o){
    Object destroy_override=find_function(E, o, "destroy");
    if(destroy_override.type!=t_null){
        Object destroy_result=call(E, destroy_override, &o, 1);
        destroy_unreferenced(E, &destroy_result);
    }
}

#undef ALREADY_DESTROYED_CHECK