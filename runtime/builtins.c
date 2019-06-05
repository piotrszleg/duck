#include "builtins.h"

object builtin_print(executor* Ex, object* arguments, int arguments_count){
    USING_STRING(stringify(Ex, arguments[0]),
        printf("%s\n", str));
    return null_const;
}

object builtin_input(executor* Ex, object* arguments, int arguments_count){
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

object builtin_substring(executor* Ex, object* arguments, int arguments_count){
    object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    object start=arguments[1];
    REQUIRE_TYPE(start, t_number)
    object end=arguments[2];
    REQUIRE_TYPE(end, t_number)
    REQUIRE(start.value<end.value, multiple_causes(Ex, (object[]){str, start, end}, 3))
    REQUIRE(start.value>=0, multiple_causes(Ex, (object[]){str, start}, 2))
    REQUIRE(end.value<=strlen(str.text), multiple_causes(Ex, (object[]){str, end}, 2))
    int length=end.value-start.value;
    char* result=malloc(sizeof(char)*(length+1));
    memcpy(result, str.text+(int)start.value, length);
    result[length]='\0';
    return to_string(result);
}

object builtin_string_length(executor* Ex, object* arguments, int arguments_count){
    object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    return to_number(strlen(str.text));
}

object builtin_to_character(executor* Ex, object* arguments, int arguments_count){
    object n=arguments[0];
    REQUIRE_TYPE(n, t_number);
    char* result=malloc(sizeof(char)*2);
    result[0]=n.value;
    result[1]='\0';
    return to_string(result);
}

object builtin_from_character(executor* Ex, object* arguments, int arguments_count){
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

object builtin_format(executor* Ex, object* arguments, int arguments_count){
    object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    stream s;
    init_stream(&s, 64);
    int variadic_counter=0;
    int str_length=strlen(str.text);

    #define NEXT_OBJECT \
        variadic_counter++; \
        if(variadic_counter>=arguments_count){ \
            RETURN_ERROR("FORMATTING_ERROR", multiple_causes(Ex, arguments, arguments_count), "Not enough arguments provided to format function."); \
        }

    for(int i=0; i<str_length; i++){
        #define COUNT_STR(s) (sizeof(s)/sizeof(char)-1)
        #define MATCH(s) (i+str_length>=COUNT_STR(s) && str_match(str.text+i, s, COUNT_STR(s)))
        if(MATCH("{}")){
            NEXT_OBJECT
            USING_STRING(stringify(Ex, arguments[variadic_counter]),
                stream_push(&s, str, strlen(str)*sizeof(char)))
            i++;
        } else if(MATCH("\\{}")) {
            stream_push(&s, &str.text[i+1], sizeof(char)*2);
            i+=2;
        } else if(MATCH("\\\\{}")){
            stream_push(&s, "\\", sizeof(char));
            NEXT_OBJECT
            USING_STRING(stringify(Ex, arguments[variadic_counter]),
                stream_push(&s, str, strlen(str)*sizeof(char)))
            i+=3;
        } else {
            stream_push(&s, &str.text[i], sizeof(char));
        }
        #undef COUNT_STR
        #undef MATCH
    }
    stream_push(&s, "\0", sizeof(char));
    return to_string(stream_get_data(&s));
}

object builtin_assert(executor* Ex, object* arguments, int arguments_count){
    object tested=arguments[0];
    if(is_falsy(tested)){
        USING_STRING(stringify(Ex, tested), 
            RETURN_ERROR("ASSERTION_FAILED", tested, "Assertion failed, %s is falsy.", str));
    } else {
        return null_const;
    }
}

object builtin_typeof(executor* Ex, object* arguments, int arguments_count){
    object self=arguments[0];
    object type_name;
    string_init(&type_name);
    type_name.text=strdup(OBJECT_TYPE_NAMES[self.type]);
    return type_name;
}

object builtin_native_get(executor* Ex, object* arguments, int arguments_count){
    object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    object key =arguments[1];
    return get_table(self.tp, key);
}

object builtin_native_set(executor* Ex, object* arguments, int arguments_count){
    object self =arguments[2];
    REQUIRE_TYPE(self, t_table)
    object key  =arguments[1]; 
    object value=arguments[2];
    USING_STRING(stringify(Ex, key),
        set_table(Ex, self.tp, to_string(str), value));
    return value;
}

object builtin_native_stringify(executor* Ex, object* arguments, int arguments_count){
    object result;
    string_init(&result);
    result.text=stringify_object(Ex, arguments[0]);
    return result;
}

object builtin_string(executor* Ex, object* arguments, int arguments_count){
    return cast(Ex, arguments[0], t_string);
}

object builtin_number(executor* Ex, object* arguments, int arguments_count){
    return cast(Ex, arguments[0], t_number);
}

object builtin_cast(executor* Ex, object* arguments, int arguments_count){
    REQUIRE_TYPE(arguments[1], t_string);
    for(int i=0; i<OBJECT_TYPE_NAMES_COUNT; i++){
        if(strcmp(OBJECT_TYPE_NAMES[i], arguments[1].text)==0){
            return cast(Ex, arguments[0], (object_type)i);
        }
    }
    RETURN_ERROR("INCORRECT_ARGUMENT", arguments[1], "Incorrect type name");
}

object builtin_native_call(executor* Ex, object* arguments, int arguments_count){
    object self=arguments[0];
    // call function omitting the first argument, because it was the function object
    return call(Ex, self, arguments+1, arguments_count-1);
}

object builtin_iterator(executor* Ex, object* arguments, int arguments_count){
    object self=arguments[0];
    REQUIRE_TYPE(self, t_table);
    // TODO
    return null_const;
}

object builtin_test(executor* Ex, object* arguments, int arguments_count){
    for(int i=0; i<arguments_count; i++){
        USING_STRING(stringify_object(Ex, arguments[i]),
            printf("<%s>, ", str));
    }
    return null_const;
}

object evaluate_file(const char* file_name, int use_bytecode);
object builtin_include(executor* Ex, object* arguments, int arguments_count){
    object path=arguments[0];
    object result;
    REQUIRE_TYPE(path, t_string)
    result=evaluate_file(path.text, true);
    return result;
}

object evaluate_string(const char* s, bool use_bytecode);
object builtin_eval(executor* Ex, object* arguments, int arguments_count){
    object text=arguments[0];
    object result;
    REQUIRE_TYPE(text, t_string)
    result=evaluate_string(text.text, true);
    return result;
}

object file_destroy(executor* Ex, object* arguments, int arguments_count){
    object self=arguments[0];
    object pointer=get(Ex, self, to_string("pointer"));
    REQUIRE(pointer.type==t_number, pointer);
    fclose((FILE*)(int)pointer.value);
    return null_const;
}

object file_read(executor* Ex, object* arguments, int arguments_count){
    object self=arguments[0];
    object pointer=get(Ex, self, to_string("pointer"));
    REQUIRE(pointer.type==t_number, pointer)
    char *buf=malloc(255*sizeof(char));
    fgets(buf, 255, (FILE*)(int)pointer.value);
    return to_string(buf);
}

object file_write(executor* Ex, object* arguments, int arguments_count){
    object self=arguments[0];
    object text=arguments[1];
    REQUIRE_TYPE(text, t_string)
    object pointer=get(Ex, self, to_string("pointer"));
    REQUIRE(pointer.type==t_number, pointer)
    fputs(text.text, (FILE*)(int)pointer.value);
    return text;
}

object builtin_open_file(executor* Ex, object* arguments, int arguments_count){
    object filename=arguments[0];
    REQUIRE_TYPE(filename, t_string)
    object mode=arguments[1];
    REQUIRE_TYPE(mode, t_string)

    object result;
    table_init(&result);
    
    FILE* f=fopen(filename.text, mode.text);
    set(Ex, result, to_string("pointer"), to_number((float)(int)f));
    set_function(Ex, result, "read", 1, false, file_read);
    set_function(Ex, result, "write", 2, false, file_write);
    set_function(Ex, result, "destroy", 1, false, file_destroy);

    return result;
}

object builtin_remove_file(executor* Ex, object* arguments, int arguments_count){
    object filename=arguments[0];
    REQUIRE_TYPE(filename, t_string)
    remove(filename.text);
    return null_const;
}

object builtin_import_dll(executor* Ex, object* arguments, int arguments_count){
    object filename=arguments[0];
    REQUIRE_TYPE(filename, t_string)
    return import_dll(Ex, filename.text);
}

void register_builtins(executor* Ex, object scope){
    #define REGISTER_FUNCTION(f, args_count) \
        object f##_function; \
        function_init(&f##_function); \
        f##_function.fp->arguments_count=args_count; \
        f##_function.fp->native_pointer=&builtin_##f; \
        set(Ex, scope, to_string(#f), f##_function);
    
    set(Ex, scope, to_string("patches"), patching_table);
    set(Ex, scope, to_string("global"), scope);
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
    set(Ex, scope, to_string("iterator"), to_function(get_table_iterator, NULL, 1));

    set_function(Ex, scope, "format", 1, true, builtin_format);

    #undef REGISTER_FUNCTION
}

object scope_get_override(executor* Ex, object* arguments, int arguments_count){
    object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    object key=arguments[1];

    object base=self;
    object map_get_result=get_table(self.tp, key);

    // we assume that all scopes are of type table and have same get behaviour
    while(map_get_result.type==t_null){
        base=get_table(base.tp, to_string("base"));
        if(base.type!=t_table){
            return null_const;
        } else {
            map_get_result=get_table(base.tp, key);
        }
    }
    return map_get_result;
}

object scope_set_override(executor* Ex, object* arguments, int arguments_count){
    object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    object key=arguments[1];
    object value=arguments[2];

    object base=self;
    object map_get_result=get_table(self.tp, key);

    // we assume that all scopes are of type table and have same get behaviour
    while(map_get_result.type==t_null){
        base=get_table(base.tp, to_string("base"));
        if(base.type!=t_table){
            // the variable isn't in any outer scope so assignment is a declaration
            set_table(Ex, self.tp, key, value);
            return value;
        } else {
            map_get_result=get_table(base.tp, key);
        }
    }
    // variable was found in outer scope so we change it's value
    set_table(Ex, base.tp, key, value);
    return value;
}

void inherit_scope(executor* Ex, object scope, object base){
    object base_global=get(Ex, base, to_string("global"));
    if(base_global.type!=t_null){
        set(Ex, scope, to_string("global"), base_global);
    } else {
        set(Ex, scope, to_string("global"), base);
    }
    set(Ex, scope, to_string("scope"), scope);
    set(Ex, scope, to_string("base"), base);
    set_function(Ex, scope, "get", 2, false, scope_get_override);
    set_function(Ex, scope, "set", 3, false, scope_set_override);
}