#include "builtins.h"

object builtin_print(object* arguments, int arguments_count){
    USING_STRING(stringify(arguments[0]),
        printf("%s\n", str));
    return null_const;
}

// source: https://stackoverflow.com/questions/1694036/why-is-the-gets-function-so-dangerous-that-it-should-not-be-used
char* fgets_no_newline(char *buffer, size_t buflen, FILE* fp) {
    if (fgets(buffer, buflen, fp) != 0)
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        return buffer;
    }
    return 0;
}

object builtin_input(object* arguments, int arguments_count){
    #define MAX_INPUT 128
    char* input=malloc(MAX_INPUT*sizeof(char));
    if(fgets_no_newline(input, MAX_INPUT, stdin)!=NULL){
        return to_string(input);
    } else {
        return null_const;
    }
    #undef MAX_INPUT
}

// string operations

object builtin_substring(object* arguments, int arguments_count){
    object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    object start=arguments[1];
    REQUIRE_TYPE(start, t_number)
    object end=arguments[2];
    REQUIRE_TYPE(end, t_number)
    REQUIRE(start.value<end.value, multiple_causes((object[]){str, start, end}, 3))
    REQUIRE(start.value>=0, multiple_causes((object[]){str, start}, 2))
    REQUIRE(end.value<=strlen(str.text), multiple_causes((object[]){str, end}, 2))
    int length=end.value-start.value;
    char* result=malloc(sizeof(char)*(length+1));
    memcpy(result, str.text+(int)start.value, length);
    result[length]='\0';
    return to_string(result);
}

object builtin_string_length(object* arguments, int arguments_count){
    object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    return to_number(strlen(str.text));
}

object builtin_to_character(object* arguments, int arguments_count){
    object n=arguments[0];
    REQUIRE_TYPE(n, t_number);
    char* result=malloc(sizeof(char)*2);
    result[0]=n.value;
    result[1]='\0';
    return to_string(result);
}

object builtin_from_character(object* arguments, int arguments_count){
    object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    REQUIRE(strlen(str.text)==1, str)
    return to_number(str.text[0]);
}

bool str_match(char* a, char* b, int length){
    for(int i=0; i<length; i++) {
        if(a[i]!=b[i]) {
            return false;
        }
    }
    return true;
}

object builtin_format(object* arguments, int arguments_count){
    object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    stream s;
    init_stream(&s, 64);
    int variadic_counter=0;
    int str_length=strlen(str.text);

    #define NEXT_OBJECT \
        variadic_counter++; \
        if(variadic_counter>=arguments_count){ \
            RETURN_ERROR("FORMATTING_ERROR", multiple_causes(arguments, arguments_count), "Not enough arguments provided to format function."); \
        }

    for(int i=0; i<str_length; i++){
        #define COUNT_STR(s) (sizeof(s)/sizeof(char)-1)
        #define MATCH(s) (i+str_length>=COUNT_STR(s) && str_match(str.text+i, s, COUNT_STR(s)))
        if(MATCH("{}")){
            NEXT_OBJECT
            USING_STRING(stringify(arguments[variadic_counter]),
                stream_push(&s, str, strlen(str)*sizeof(char)))
            i++;
        } else if(MATCH("\\{}")) {
            stream_push(&s, &str.text[i+1], sizeof(char)*2);
            i+=2;
        } else if(MATCH("\\\\{}")){
            stream_push(&s, "\\", sizeof(char));
            NEXT_OBJECT
            USING_STRING(stringify(arguments[variadic_counter]),
                stream_push(&s, str, strlen(str)*sizeof(char)))
            i+=3;
        } else {
            stream_push(&s, &str.text[i], sizeof(char));
        }
        #undef COUNT_STR
        #undef MATCH
    }
    stream_push(&s, "\0", sizeof(char));
    return to_string(s.data);
}

object builtin_assert(object* arguments, int arguments_count){
    object tested=arguments[0];
    if(is_falsy(tested)){
        USING_STRING(stringify(tested), 
            RETURN_ERROR("ASSERTION_FAILED", tested, "Assertion failed, %s is falsy.", str));
    } else {
        return null_const;
    }
}

object builtin_typeof(object* arguments, int arguments_count){
    object self=arguments[0];
    object type_name;
    string_init(&type_name);
    type_name.text=strdup(OBJECT_TYPE_NAMES[self.type]);
    return type_name;
}

object builtin_native_get(object* arguments, int arguments_count){
    object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    object key =arguments[1];
    return get_table(self.tp, key);
}

object builtin_native_set(object* arguments, int arguments_count){
    object self =arguments[2];
    REQUIRE_TYPE(self, t_table)
    object key  =arguments[1]; 
    object value=arguments[2];
    USING_STRING(stringify(key),
        set_table(self.tp, to_string(str), value));
    return value;
}

object builtin_native_stringify(object* arguments, int arguments_count){
    object result;
    string_init(&result);
    result.text=stringify_object(arguments[0]);
    return result;
}

object builtin_string(object* arguments, int arguments_count){
    return cast(arguments[0], t_string);
}

object builtin_number(object* arguments, int arguments_count){
    return cast(arguments[0], t_number);
}

object builtin_cast(object* arguments, int arguments_count){
    REQUIRE_TYPE(arguments[1], t_string);
    for(int i=0; i<OBJECT_TYPE_NAMES_COUNT; i++){
        if(strcmp(OBJECT_TYPE_NAMES[i], arguments[1].text)==0){
            return cast(arguments[0], (object_type)i);
        }
    }
    RETURN_ERROR("INCORRECT_ARGUMENT", arguments[1], "Incorrect type name");
}

object builtin_native_call(object* arguments, int arguments_count){
    object self=arguments[0];
    // call function omitting the first argument, because it was the function object
    return call(self, arguments+1, arguments_count-1);
}

object builtin_iterator(object* arguments, int arguments_count){
    object self=arguments[0];
    REQUIRE_TYPE(self, t_table);
    // TODO
    return null_const;
}

object builtin_test(object* arguments, int arguments_count){
    for(int i=0; i<arguments_count; i++){
        USING_STRING(stringify_object(arguments[i]),
            printf("<%s>, ", str));
    }
    return null_const;
}

object evaluate_file(const char* file_name, int use_bytecode);
object builtin_include(object* arguments, int arguments_count){
    object path=arguments[0];
    object result;
    REQUIRE_TYPE(path, t_string)
    result=evaluate_file(path.text, true);
    return result;
}

object evaluate_string(const char* s, bool use_bytecode);
object builtin_eval(object* arguments, int arguments_count){
    object text=arguments[0];
    object result;
    REQUIRE_TYPE(text, t_string)
    result=evaluate_string(text.text, true);
    return result;
}

object file_destroy(object* arguments, int arguments_count){
    object self=arguments[0];
    object pointer=get(self, to_string("pointer"));
    REQUIRE(pointer.type==t_number, pointer);
    fclose((FILE*)(int)pointer.value);
    return null_const;
}

object file_read(object* arguments, int arguments_count){
    object self=arguments[0];
    object pointer=get(self, to_string("pointer"));
    REQUIRE(pointer.type==t_number, pointer)
    char *buf=malloc(255*sizeof(char));
    fgets(buf, 255, (FILE*)(int)pointer.value);
    return to_string(buf);
}

object file_write(object* arguments, int arguments_count){
    object self=arguments[0];
    object text=arguments[1];
    REQUIRE_TYPE(text, t_string)
    object pointer=get(self, to_string("pointer"));
    REQUIRE(pointer.type==t_number, pointer)
    fputs(text.text, (FILE*)(int)pointer.value);
    return text;
}

object builtin_open_file(object* arguments, int arguments_count){
    object filename=arguments[0];
    REQUIRE_TYPE(filename, t_string)
    object mode=arguments[1];
    REQUIRE_TYPE(mode, t_string)

    object result;
    table_init(&result);
    
    FILE* f=fopen(filename.text, mode.text);
    set(result, to_string("pointer"), to_number((float)(int)f));
    set_function(result, "read", 1, false, file_read);
    set_function(result, "write", 2, false, file_write);
    set_function(result, "destroy", 1, false, file_destroy);

    return result;
}

object builtin_remove_file(object* arguments, int arguments_count){
    object filename=arguments[0];
    REQUIRE_TYPE(filename, t_string)
    remove(filename.text);
    return null_const;
}

object builtin_import_dll(object* arguments, int arguments_count){
    object filename=arguments[0];
    REQUIRE_TYPE(filename, t_string)
    return import_dll(filename.text);
}

void register_builtins(object scope){
    #define REGISTER_FUNCTION(f, args_count) \
        object f##_function; \
        function_init(&f##_function); \
        f##_function.fp->arguments_count=args_count; \
        f##_function.fp->native_pointer=&builtin_##f; \
        set(scope, to_string(#f), f##_function);
    
    set(scope, to_string("patches"), patching_table);
    set(scope, to_string("global"), scope);
    REGISTER_FUNCTION(print, 1)
    REGISTER_FUNCTION(input, 0)
    REGISTER_FUNCTION(assert, 1)
    REGISTER_FUNCTION(typeof, 1)
    REGISTER_FUNCTION(native_get, 2)
    REGISTER_FUNCTION(native_call, 2)
    REGISTER_FUNCTION(native_stringify, 1)
    REGISTER_FUNCTION(include, 1)
    REGISTER_FUNCTION(eval, 1)
    REGISTER_FUNCTION(substring, 3)
    REGISTER_FUNCTION(string_length, 1)
    REGISTER_FUNCTION(from_character, 1)
    REGISTER_FUNCTION(to_character, 1)
    REGISTER_FUNCTION(string, 1)
    REGISTER_FUNCTION(number, 1)
    REGISTER_FUNCTION(cast, 2)
    REGISTER_FUNCTION(open_file, 2)
    REGISTER_FUNCTION(remove_file, 1)
    REGISTER_FUNCTION(import_dll, 1)
    //REGISTER_FUNCTION(test, 2);
    set(scope, to_string("iterator"), to_function(get_table_iterator, NULL, 1));

    object format_function;
    function_init(&format_function);
    format_function.fp->variadic=true;
    format_function.fp->arguments_count=2;
    format_function.fp->native_pointer=&builtin_format;
    set(scope, to_string("format"), format_function);

    #undef REGISTER_FUNCTION
}

object scope_get_override(object* arguments, int arguments_count){
    object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    object key=arguments[1];
    object map_get_result=get_table(self.tp, key);

    if(map_get_result.type!=t_null){
        return map_get_result;
    } else{
        object base=get_table(self.tp, to_string("base"));
        if(base.type==t_table){
            return get(base, key);
        }
        return null_const;
    }
}

void inherit_scope(object scope, object base){
    object f;
    function_init(&f);
    f.fp->native_pointer=&scope_get_override;
    f.fp->arguments_count=2;
    object base_global=get(base, to_string("global"));
    if(base_global.type!=t_null){
        set(scope, to_string("global"), base_global);
    } else {
        set(scope, to_string("global"), base);
        dereference(&base_global);// base_global is null so it can be safely deleted
    }
    set(scope, to_string("get"), f);
    set(scope, to_string("scope"), scope);
    set(scope, to_string("base"), base);
}