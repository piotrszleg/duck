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

static Object get_patch(Executor* E, const char* type_name, const char* patch_name) {
    Object patching_table=executor_get_patching_table(E);
    if(patching_table.type==t_null){
        return null_const;
    }
    REQUIRE_TYPE(patching_table, t_table)
    Object type_patching_table=table_get(E, patching_table.tp, to_string(type_name));
    if(type_patching_table.type==t_null){
        return null_const;
    }
    REQUIRE_TYPE(type_patching_table, t_table)
    return table_get(E, type_patching_table.tp, to_string(patch_name));
}

#define PATCH(patch_name, object_type, ...) \
    Object patch=get_patch(E, OBJECT_TYPE_NAMES[object_type], patch_name); \
    if(patch.type!=t_null){ \
        Object arguments[]={__VA_ARGS__}; \
        return call(E, patch, arguments, sizeof(arguments)/sizeof(Object)); \
    }

static Object table_get_override(Executor* E, Object o, const char* override_name) {
    if(table_has_special_fields(o.tp)){
        Object override=get(E, o, to_string(override_name));
        return override;
    } else {
        return null_const;
    }
}

bool cast_is_constant(ObjectType from, ObjectType to) {
    if(from==to) {
        return true;
    }
    switch(to) {
        case t_string: return true;
        case t_int:
            return from==t_float||from==t_null;
        case t_float:
            return from==t_int||from==t_null;
        default:
            return false;
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
        case t_table: {
            char* override_name=string_add("to_", OBJECT_TYPE_NAMES[o.type]);
            Object cast_override=table_get_override(E, o, override_name);
            if(cast_override.type!=t_null){
                // call get_function a and b as arguments
                Object result=call(E, cast_override, &o, 1);
                destroy_unreferenced(E, &cast_override);
                free(override_name);
                return result;
            }
            destroy_unreferenced(E, &cast_override);
            free(override_name);
            break;
        }
        default:;
    }
    char* patch_name=string_add("to_", OBJECT_TYPE_NAMES[o.type]);
    Object patch=get_patch(E, OBJECT_TYPE_NAMES[o.type], patch_name);
    if(patch.type!=t_null){
        free(patch_name);
        return call(E, patch, &o, 1);
    }
    free(patch_name);
    RETURN_ERROR("TYPE_CONVERSION_FAILURE", o, "Can't convert from <%s> to <%s>", OBJECT_TYPE_NAMES[o.type], OBJECT_TYPE_NAMES[type]);
}

int compare(Executor* E, Object a, Object b){
    Object error=null_const;
    int comparison_result=compare_and_get_error(E, a, b, &error); \
    if(error.type!=t_null) {
        destroy_unreferenced(E, &error);
    }
    return comparison_result;
}

bool compare_is_constant(ObjectType a, ObjectType b) {
    if(a==t_null||b==t_null){
        return true;
    }
    if((a==t_int||a==t_float)&&(b==t_int||b==t_float)){
        return true;
    }
    if(a!=b){
        return false;
    }
    switch(a) {
        case t_string:
        case t_pointer:
        case t_gc_pointer:
        case t_function:
            return true;
        default:
            return false;
    }
}

// if a>b returns 1 if a<b returns -1, if a==b returns 0
int compare_and_get_error(Executor* E, Object a, Object b, Object* error){
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
        NEW_ERROR(*error, "COMPARISON_ERROR", multiple_causes(E, (Object[]){a, b}, 2), "Can't compare objects of different types.");
        return 1;
    }
    switch(a.type){
        case t_string:
            return strcmp(a.text, b.text);
        case t_int:
            return sign(a.int_value-b.int_value);
        case t_float:
            return sign(a.float_value-b.float_value);
        case t_table: {
            Object compare_override=table_get_override(E, a, "compare");
            if(compare_override.type!=t_null){
                Object call_result=call(E, compare_override, (Object[]){a, b}, 2);
                destroy_unreferenced(E, &compare_override);
                if(call_result.type==t_int){
                    return call_result.int_value;
                } else {
                    NEW_ERROR(*error, "COMPARISON_ERROR", multiple_causes(E, (Object[]){a, b, call_result}, 3), "Function at field compare didn't return an int value.");
                    return 1;
                }
            } else {
                destroy_unreferenced(E, &compare_override);
                return table_compare(E, a.tp, b.tp, error);
            }
        }
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
            NEW_ERROR(*error, "COMPARISON_ERROR", multiple_causes(E, (Object[]){a, b}, 2), "Can't compare objects of type %s", OBJECT_TYPE_NAMES[a.type]);
            return 1;
    }
}

static unsigned hash_string(const char *str) {
    unsigned hashed = 5381;
    while (*str) {
      hashed = ((hashed << 5) + hashed) ^ *str++;
    }
    return hashed;
}

unsigned hash(Executor* E, Object o, Object* error) {
    switch(o.type){
        case t_string:
            return hash_string(o.text);
        case t_int:
            return o.int_value;
        case t_float:
            return o.float_value;
        case t_null:
            return 0;
        case t_table: {
            Object hash_override=table_get_override(E, o, "hash");
            if(hash_override.type!=t_null){
                Object call_result=call(E, hash_override, &o, 1);
                destroy_unreferenced(E, &hash_override);
                if(call_result.type!=t_int){
                    NEW_ERROR(*error, "HASH_ERROR", multiple_causes(E, (Object[]){o, call_result}, 2), "Function at field hash didn't return an int value.");
                    return 0;
                } else if(call_result.int_value<0){
                    NEW_ERROR(*error, "HASH_ERROR", multiple_causes(E, (Object[]){o, call_result}, 2), "Function at field hash returned a negative value.");
                    return 0;
                } else {
                    return call_result.int_value;
                }
            } else {
                destroy_unreferenced(E, &hash_override);
                return table_hash(E, o.tp, error);
            }
        }
        case t_function:
            switch(o.fp->ftype){
                case f_ast:
                case f_bytecode:
                    return (unsigned)o.fp->source_pointer;
                case f_special:
                    return (unsigned)o.fp->special_index;
                case f_native:
                    return (unsigned)o.fp->native_pointer;
            }
         case t_pointer:
            return (unsigned)o.p;
        case t_gc_pointer:
            return (unsigned)o.gcp;
        default:
            NEW_ERROR(*error, "HASH_ERROR", o, "Can't hash object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
            return 0;
    }
}

Object coroutine_iterator_next(Executor* E, Object scope, Object* arguments, int arguments_count){
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

Object string_iterator_next(Executor* E, Object scope, Object* arguments, int arguments_count){
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
            Object iterator_override=table_get_override(E, o, "iterator");
            if(iterator_override.type!=t_null){
                return call(E, iterator_override, &o, 1);
            } else {
                return table_get_iterator_object(E, null_const, &o, 1);
            }
        }
        case t_coroutine:
            return coroutine_iterator(E, o);
        case t_string:
            return string_iterator(E, o);
        default:;
    }
    PATCH("iterator", o.type, o)
    RETURN_ERROR("ITERATION_ERROR", o, "Can't get iterator of object of type %s.", OBJECT_TYPE_NAMES[o.type]);
}

bool operator_is_constant(ObjectType a, ObjectType b, const char* op) {
    if(a==t_table){
        return false;
    }
    size_t op_length=strlen(op);
    if(op_length==1) {
        switch(op[0]){
            case '!': return true;
            case '-': return true;
            case '#': return false;
        }
        #define CASE(character) \
            case character: \
                return cast_is_constant(b, a);
        if(a==t_int) {
            switch(op[0]){
                CASE('+')
                CASE('-')
                CASE('*')
                CASE('%')
                CASE('/')
            }
        }
        if(a==t_float) {
            switch(op[0]){
                CASE('+')
                CASE('-')
                CASE('*')
                CASE('/')
            }
        }
        #undef CASE
    } else {  
        if(strcmp(op, "--")==0) return false;
        if(strcmp(op, "><")==0) return false;
        if(strcmp(op, "##")==0) return false;
        if(strcmp(op, "&&")==0) return true;
        if(strcmp(op, "||")==0) return true;
        if(strcmp(op, "//")==0) return a==t_int && cast_is_constant(b, a);
        #define COMPARISSON(operator_name) if(strcmp(op, operator_name)==0) return compare_is_constant(a, b);
        COMPARISSON("==") 
        COMPARISSON("!=")
        COMPARISSON(">")
        COMPARISSON("<")
        COMPARISSON(">=")
        COMPARISSON("<=")
        COMPARISSON("is")
        COMPARISSON("compare")
        #undef COMPARISSON
    }
    return false;
}

Object operator(Executor* E, Object a, Object b, const char* op){
    size_t op_length=strlen(op);
    if(a.type==t_table){
        Object operator_override=table_get_override(E, a, op);
        if(operator_override.type!=t_null){
            // call get_function a and b as arguments
            Object result=call(E, operator_override, ((Object[]){a, b}), 2);
            destroy_unreferenced(E, &operator_override);
            return result;
        }
        destroy_unreferenced(E, &operator_override);
    }
    #define OP_CASE(operator_name) if(strcmp(op, operator_name)==0)
    #define COMPARISON_OPERATOR(check) { \
        Object error=null_const; \
        int comparison_result=compare_and_get_error(E, a, b, &error); \
        if(error.type!=t_null){ \
            return error; \
        } else { \
            return to_int(check); \
        } \
    }
    #define COMPARISON_OPERATOR_CASE(operator_name, check) \
        OP_CASE(operator_name) { \
            COMPARISON_OPERATOR(check) \
        }
    if(op_length==1){
        // call b with iterator result's value for each iteration
        switch(op[0]){
            case '#': {
                Object it;
                Object call_result=null_const;
                FOREACH(a, it, 
                    call_result=call(E, b, (Object[]){get(E, it, to_string("value"))}, 1);
                )
                return call_result;
            }
            case '!':
                return to_int(is_falsy(b));
            case '-':
                if(a.type==t_null && b.type==t_int){
                    return to_int(-b.int_value);
                } else if(a.type==t_null && b.type==t_float){
                    return to_int(-b.float_value);
                } else {
                    break;
                }
            case '>':
                COMPARISON_OPERATOR(comparison_result==1)
            case '<':
                COMPARISON_OPERATOR(comparison_result==-1)
        }
    } else if(op_length==2){
        COMPARISON_OPERATOR_CASE("==", comparison_result==0)
        COMPARISON_OPERATOR_CASE("!=", comparison_result!=0)
        COMPARISON_OPERATOR_CASE(">=", comparison_result==1||comparison_result==0)
        COMPARISON_OPERATOR_CASE("<=", comparison_result==-1||comparison_result==0)
        OP_CASE("is"){
            if(a.type!=b.type){
                return to_int(0);
            } else if(is_gc_object(a)) {
                return to_int(a.gco==b.gco);
            } else {
                Object error;
                int comparison_result=compare_and_get_error(E, a, b, &error);
                if(error.type!=t_null){
                    return error;
                } else {
                    return to_int(comparison_result==0);
                }
            }
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
    } else {
        COMPARISON_OPERATOR_CASE("compare", comparison_result)
    }
    Object b_casted;
    #define CAST_B \
        if(b.type!=a.type){/* checking it here avoids unecessary function call */ \
            Object b_casted=cast(E, b, a.type); \
            if(b_casted.type!=a.type){ \
                return b_casted;/* b_casted is conversion error */ \
            } \
        } else { \
            b_casted=b; \
        }
    switch(a.type){
        case t_string:
            if(op_length!=1){
                break;
            }
            switch(op[0]){
                case '+': {
                    Object result;
                    if(b.type!=t_string) {
                        USING_STRING(stringify(E, b),
                            result=to_string(string_add(a.text, str)))
                    } else {
                        result=to_string(string_add(a.text, b.text));
                    }
                    return result;
                }
                case '*':
                    if(b.type==t_int && b.int_value>0){
                        return to_string(string_repeat(a.text, b.int_value));
                    }
                    break;
            }
            break;
        case t_int:{
            if(op_length==2){
                if(op[0]=='/' && op[1]=='/'){
                    CAST_B
                    return to_int(a.int_value/b_casted.int_value);
                }
            } else if(op_length==1){
                switch(op[0]){
                    case '+':
                        CAST_B
                        return to_int(a.int_value+b_casted.int_value);
                    case '-':
                        CAST_B
                        return to_int(a.int_value-b_casted.int_value);
                    case '*':
                        CAST_B
                        return to_int(a.int_value*b_casted.int_value);
                    case '%':
                        CAST_B
                        return to_int(a.int_value%b_casted.int_value);
                    case '/':
                        CAST_B
                        return to_float((float)a.int_value/b_casted.int_value);
                }
            }
            destroy_unreferenced(E, &b_casted);
            break;
        }
        case t_float: {
            if(op_length==1){
                switch(op[0]){
                    case '+':
                        CAST_B
                        return to_float(a.float_value+b_casted.float_value);
                    case '-':
                        CAST_B
                        return to_float(a.float_value-b_casted.float_value);
                    case '*':
                        CAST_B
                        return to_float(a.float_value*b_casted.float_value);
                    case '/':
                        CAST_B
                        return to_float((float)a.int_value/b_casted.int_value);
                }
                destroy_unreferenced(E, &b_casted);
            }
            break;
        }
        default:;
    }
    PATCH(op, a.type, a, b);
    RETURN_ERROR("OPERATOR_ERROR", multiple_causes(E, (Object[]){a, b}, 2), "Can't perform operotion '%s' on objects of type <%s> and <%s>", op, OBJECT_TYPE_NAMES[a.type], OBJECT_TYPE_NAMES[b.type]);
}

bool is_serializable(Object o) {
    switch(o.type) {
        case t_string:
        case t_float:
        case t_int:
        case t_null:
        case t_table:
            return true;
        default:
            return false;
    }
}

char* serialize(Executor* E, Object o) {
    if(o.type==t_table){
        Object serialize_override=table_get_override(E, o, "serialize");
        if(serialize_override.type!=t_null){
            Object result=call(E, serialize_override, &o, 1);
            if(result.type!=t_string){
                return stringify_object(E, result);
            } else {
                return result.text;
            }
        } else {
            return table_serialize(E, o.tp);
        }
    } else if(is_serializable(o)){
        return stringify_object(E, o);
    } else {
        return strdup("");
    }
}

char* stringify(Executor* E, Object o){
    if(o.type==t_table){
        Object stringify_override=table_get_override(E, o, "stringify");
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
            return table_stringify(E, o.tp);
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
                if(f->variadic){
                    BUFFER_WRITE("...", 3);
                }
                BUFFER_WRITE(")", 1);
                
                buffer[buffer_size-1]='\0';// to make sure that the string won't overflow

                char* buffer_truncated=strdup(buffer);
                free(buffer);
                return buffer_truncated;
            } else if(f->arguments_count>0){
                char* buffer=malloc(16*sizeof(char*));
                if(f->variadic){
                    snprintf(buffer, 16, "function(%i...)", f->arguments_count);
                } else {
                    snprintf(buffer, 16, "function(%i)", f->arguments_count);
                }
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
            // try to get "get" field overriding function from the table and use it
            Object get_override=null_const;
            if(table_has_special_fields(o.tp)){
                get_override=table_get(E, o.tp, to_string("get"));
            }
            if(get_override.type!=t_null){
                Object arguments[]={o, key};
                Object result=call(E, get_override, arguments, 2);
                return result;
            } else {
                return table_get(E, o.tp, key);
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
    PATCH("get", o.type, o, key);
    RETURN_ERROR("GET_ERROR",  multiple_causes(E, (Object[]){o, key}, 2), "Can't get field in object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
}

Object set(Executor* E, Object o, Object key, Object value){
    if(o.type==t_table){
        // try to get "get" operator overriding function from the Table and use it
        Object set_override=table_get_override(E, o, "set");
        if(set_override.type!=t_null){
            return call(E, set_override, (Object[]){o, key, value}, 3);
        } else if(table_is_protected(o.tp)){
            RETURN_ERROR("SET_ERROR", multiple_causes(E, (Object[]){o, key, value}, 3), 
                         "Attempted to set a field in a protected table.");
        } else {
            table_set(E, o.tp, key, value);
            return value;
        }
    } else {
        PATCH("set", o.type, o, key);
        RETURN_ERROR("SET_ERROR", o, "Can't set field in object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
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
            Object call_override=table_get_override(E, o, "call");
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
        default: {
            Object patch=get_patch(E, OBJECT_TYPE_NAMES[o.type], "call");
            if(patch.type!=t_null){
                Object* arguments_with_self=malloc(sizeof(Object)*(arguments_count+1));
                arguments_with_self[0]=o;
                memcpy(arguments_with_self+1, arguments, arguments_count*sizeof(Object));
                Object result=call(E, patch, arguments_with_self, arguments_count+1);
                free(arguments_with_self);
                return result;
            } else {
                RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Can't call object of type <%s>", OBJECT_TYPE_NAMES[o.type]);
            }
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
            Object copy_override=table_get_override(E, o, "copy");
            if(copy_override.type!=t_null){
                return call(E, copy_override, &o, 1);
            } else {
                return table_copy(E, o.tp);
            }
        }
        default:
            return o;
    }
}

void call_destroy(Executor* E, Object o){
    Object destroy_override=table_get_override(E, o, "destroy");
    if(destroy_override.type!=t_null){
        Object destroy_result=call(E, destroy_override, &o, 1);
        destroy_unreferenced(E, &destroy_result);
    }
}

#undef ALREADY_DESTROYED_CHECK