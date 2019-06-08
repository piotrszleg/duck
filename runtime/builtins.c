#include "builtins.h"

Object builtin_coroutine(Executor* E, Object* arguments, int arguments_count){
    Object function=arguments[0];
    Object coroutine;
    coroutine_init(E, &coroutine);
    
    Executor* coroutine_executor=malloc(sizeof(Executor));

    // copy bytecode program
    coroutine_executor->options=E->options;
    coroutine_executor->bytecode_environment.main_program=malloc(sizeof(BytecodeProgram));
    bytecode_program_copy(E->bytecode_environment.main_program, coroutine_executor->bytecode_environment.main_program);
    bytecode_environment_init(&coroutine_executor->bytecode_environment);

    // coroutine shares garbage collector with owner
    coroutine_executor->gc=E->gc;

    // create a coroutine scope inheriting from global scope
    table_init(coroutine_executor, &coroutine_executor->bytecode_environment.scope);
    inherit_scope(coroutine_executor, coroutine_executor->bytecode_environment.scope, get(E, E->bytecode_environment.scope, to_string("global")));

    coroutine_executor->coroutine=coroutine.co;
    coroutine.co->state=co_uninitialized;

    // pass arguments and move to given function but don't call it yet
    REQUIRE_TYPE(function, t_function)
    REQUIRE(function.fp->ftype==f_bytecode, function)
    coroutine_executor->bytecode_environment.executed_program=function.fp->source_pointer;
    REQUIRE(function.fp->arguments_count==arguments_count-1, function)
    for(int i=1; i<arguments_count; i++){
        push(&coroutine_executor->bytecode_environment.object_stack, arguments[i]);
    }

    coroutine.co->executor=coroutine_executor;
    return coroutine;
}

Object coroutine_call(Executor* E, Coroutine* coroutine, Object* arguments, int arguments_count){
    switch(coroutine->state){
        case co_uninitialized:
            if(arguments_count!=0){
                RETURN_ERROR("COROUTINE_ERROR", wrap_gc_object((gc_Object*)coroutine), "First call to coroutine shouldn't have any arguments, %i given", arguments_count)
            } else {
                coroutine->state=co_running;
                return execute_bytecode(coroutine->executor);
            }
        case co_running:
            // coroutine expects one value to be emitted from yield call
            if(arguments_count==1){
                push(&coroutine->executor->bytecode_environment.object_stack, arguments[0]);
            } else if(arguments_count==0){
                push(&coroutine->executor->bytecode_environment.object_stack, null_const);
            } else {
                RETURN_ERROR("COROUTINE_ERROR", wrap_gc_object((gc_Object*)coroutine), "Coroutines can accept either zero or one argument, %i given", arguments_count)
            }
            return execute_bytecode(coroutine->executor);
        case co_finished: return null_const;
        default: RETURN_ERROR("COROUTINE_ERROR", wrap_gc_object((gc_Object*)coroutine), "Unknown coroutine state: %i", coroutine->state)
    }
}

Object builtin_print(Executor* E, Object* arguments, int arguments_count){
    USING_STRING(stringify(E, arguments[0]),
        printf("%s\n", str));
    return null_const;
}

Object builtin_input(Executor* E, Object* arguments, int arguments_count){
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

Object builtin_substring(Executor* E, Object* arguments, int arguments_count){
    Object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    Object start=arguments[1];
    REQUIRE_TYPE(start, t_number)
    Object end=arguments[2];
    REQUIRE_TYPE(end, t_number)
    REQUIRE(start.value<end.value, multiple_causes(E, (Object[]){str, start, end}, 3))
    REQUIRE(start.value>=0, multiple_causes(E, (Object[]){str, start}, 2))
    REQUIRE(end.value<=strlen(str.text), multiple_causes(E, (Object[]){str, end}, 2))
    int length=end.value-start.value;
    char* result=malloc(sizeof(char)*(length+1));
    memcpy(result, str.text+(int)start.value, length);
    result[length]='\0';
    return to_string(result);
}

Object builtin_string_length(Executor* E, Object* arguments, int arguments_count){
    Object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    return to_number(strlen(str.text));
}

Object builtin_to_character(Executor* E, Object* arguments, int arguments_count){
    Object n=arguments[0];
    REQUIRE_TYPE(n, t_number);
    char* result=malloc(sizeof(char)*2);
    result[0]=n.value;
    result[1]='\0';
    return to_string(result);
}

Object builtin_from_character(Executor* E, Object* arguments, int arguments_count){
    Object str=arguments[0];
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

Object builtin_format(Executor* E, Object* arguments, int arguments_count){
    Object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    stream s;
    init_stream(&s, 64);
    int variadic_counter=0;
    int str_length=strlen(str.text);

    #define NEXT_OBJECT \
        variadic_counter++; \
        if(variadic_counter>=arguments_count){ \
            RETURN_ERROR("FORMATTING_ERROR", multiple_causes(E, arguments, arguments_count), "Not enough arguments provided to format function."); \
        }

    for(int i=0; i<str_length; i++){
        #define COUNT_STR(s) (sizeof(s)/sizeof(char)-1)
        #define MATCH(s) (i+str_length>=COUNT_STR(s) && str_match(str.text+i, s, COUNT_STR(s)))
        if(MATCH("{}")){
            NEXT_OBJECT
            USING_STRING(stringify(E, arguments[variadic_counter]),
                stream_push(&s, str, strlen(str)*sizeof(char)))
            i++;
        } else if(MATCH("\\{}")) {
            stream_push(&s, &str.text[i+1], sizeof(char)*2);
            i+=2;
        } else if(MATCH("\\\\{}")){
            stream_push(&s, "\\", sizeof(char));
            NEXT_OBJECT
            USING_STRING(stringify(E, arguments[variadic_counter]),
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

Object builtin_assert(Executor* E, Object* arguments, int arguments_count){
    Object tested=arguments[0];
    if(is_falsy(tested)){
        USING_STRING(stringify(E, tested), 
            RETURN_ERROR("ASSERTION_FAILED", tested, "Assertion failed, %s is falsy.", str));
    } else {
        return null_const;
    }
}

Object builtin_typeof(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object type_name;
    string_init(&type_name);
    type_name.text=strdup(OBJECT_TYPE_NAMES[self.type]);
    return type_name;
}

Object builtin_native_get(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    Object key =arguments[1];
    return get_table(self.tp, key);
}

Object builtin_native_set(Executor* E, Object* arguments, int arguments_count){
    Object self =arguments[2];
    REQUIRE_TYPE(self, t_table)
    Object key  =arguments[1]; 
    Object value=arguments[2];
    USING_STRING(stringify(E, key),
        set_table(E, self.tp, to_string(str), value));
    return value;
}

Object builtin_native_stringify(Executor* E, Object* arguments, int arguments_count){
    Object result;
    string_init(&result);
    result.text=stringify_object(E, arguments[0]);
    return result;
}

Object builtin_string(Executor* E, Object* arguments, int arguments_count){
    return cast(E, arguments[0], t_string);
}

Object builtin_number(Executor* E, Object* arguments, int arguments_count){
    return cast(E, arguments[0], t_number);
}

Object builtin_cast(Executor* E, Object* arguments, int arguments_count){
    REQUIRE_TYPE(arguments[1], t_string);
    for(int i=0; i<OBJECT_TYPE_NAMES_COUNT; i++){
        if(strcmp(OBJECT_TYPE_NAMES[i], arguments[1].text)==0){
            return cast(E, arguments[0], (ObjectType)i);
        }
    }
    RETURN_ERROR("INCORRECT_ARGUMENT", arguments[1], "Incorrect type name");
}

Object builtin_native_call(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    // call function omitting the first argument, because it was the function Object
    return call(E, self, arguments+1, arguments_count-1);
}

Object builtin_iterator(Executor* E, Object* arguments, int arguments_count){
    Object o=arguments[0];
    return get_iterator(E, o);
}

Object builtin_test(Executor* E, Object* arguments, int arguments_count){
    for(int i=0; i<arguments_count; i++){
        USING_STRING(stringify_object(E, arguments[i]),
            printf("<%s>, ", str));
    }
    return null_const;
}

// TOFIX: don't use current scope
Object evaluate_file(Executor* E, const char* file_name, Object scope);
Object builtin_include(Executor* E, Object* arguments, int arguments_count){
    Object path=arguments[0];
    Object result;
    REQUIRE_TYPE(path, t_string)
    result=evaluate_file(E, path.text, E->bytecode_environment.scope);
    return result;
}

Object evaluate_string(Executor* E, const char* s, Object scope);
Object builtin_eval(Executor* E, Object* arguments, int arguments_count){
    Object text=arguments[0];
    Object result;
    REQUIRE_TYPE(text, t_string)
    result=evaluate_string(E, text.text, E->bytecode_environment.scope);
    return result;
}

Object file_destroy(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object pointer=get(E, self, to_string("pointer"));
    REQUIRE(pointer.type==t_number, pointer);
    fclose((FILE*)(int)pointer.value);
    return null_const;
}

Object file_read(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object pointer=get(E, self, to_string("pointer"));
    REQUIRE(pointer.type==t_number, pointer)
    char *buf=malloc(255*sizeof(char));
    fgets(buf, 255, (FILE*)(int)pointer.value);
    return to_string(buf);
}

Object file_write(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object text=arguments[1];
    REQUIRE_TYPE(text, t_string)
    Object pointer=get(E, self, to_string("pointer"));
    REQUIRE(pointer.type==t_number, pointer)
    fputs(text.text, (FILE*)(int)pointer.value);
    return text;
}

Object builtin_open_file(Executor* E, Object* arguments, int arguments_count){
    Object filename=arguments[0];
    REQUIRE_TYPE(filename, t_string)
    Object mode=arguments[1];
    REQUIRE_TYPE(mode, t_string)

    Object result;
    table_init(E, &result);
    
    FILE* f=fopen(filename.text, mode.text);
    set(E, result, to_string("pointer"), to_number((float)(int)f));
    set_function(E, result, "read", 1, false, file_read);
    set_function(E, result, "write", 2, false, file_write);
    set_function(E, result, "destroy", 1, false, file_destroy);

    return result;
}

Object builtin_remove_file(Executor* E, Object* arguments, int arguments_count){
    Object filename=arguments[0];
    REQUIRE_TYPE(filename, t_string)
    remove(filename.text);
    return null_const;
}

Object builtin_import_dll(Executor* E, Object* arguments, int arguments_count){
    Object filename=arguments[0];
    REQUIRE_TYPE(filename, t_string)
    return import_dll(E, filename.text);
}

void register_builtins(Executor* E, Object scope){
    #define REGISTER_FUNCTION(f, args_count) \
        Object f##_function; \
        function_init(E, &f##_function); \
        f##_function.fp->arguments_count=args_count; \
        f##_function.fp->native_pointer=&builtin_##f; \
        set(E, scope, to_string(#f), f##_function);
    
    set(E, scope, to_string("global"), scope);
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
    REGISTER_FUNCTION(iterator, 1)

    Object yield;
    function_init(E, &yield);
    yield.fp->ftype=f_special;
    yield.fp->special_index=0;
    set(E, scope, to_string("yield"), yield);

    set_function(E, scope, "table_iterator", 1, false, get_table_iterator);

    set_function(E, scope, "coroutine", 1, true, builtin_coroutine);
    set_function(E, scope, "format", 1, true, builtin_format);

    #undef REGISTER_FUNCTION
}

Object scope_get_override(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    Object key=arguments[1];

    Object base=self;
    Object map_get_result=get_table(self.tp, key);

    // we assume that all scopes are of type Table and have same get behaviour
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

Object scope_set_override(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    Object key=arguments[1];
    Object value=arguments[2];

    Object base=self;
    Object map_get_result=get_table(self.tp, key);

    // we assume that all scopes are of type Table and have same get behaviour
    while(map_get_result.type==t_null){
        base=get_table(base.tp, to_string("base"));
        if(base.type!=t_table){
            // the variable isn't in any outer scope so assignment is a declaration
            set_table(E, self.tp, key, value);
            return value;
        } else {
            map_get_result=get_table(base.tp, key);
        }
    }
    // variable was found in outer scope so we change it's value
    set_table(E, base.tp, key, value);
    return value;
}

void inherit_scope(Executor* E, Object scope, Object base){
    Object base_global=get(E, base, to_string("global"));
    if(base_global.type!=t_null){
        set(E, scope, to_string("global"), base_global);
    } else {
        set(E, scope, to_string("global"), base);
    }
    set(E, scope, to_string("scope"), scope);
    set(E, scope, to_string("base"), base);
    set_function(E, scope, "get", 2, false, scope_get_override);
    set_function(E, scope, "set", 3, false, scope_set_override);
}