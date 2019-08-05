#include "builtins.h"

Object builtin_coroutine(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object function=arguments[0];
    Object coroutine;
    coroutine_init(E, &coroutine);
    
    Executor* coroutine_executor=malloc(sizeof(Executor));

    // copy bytecode program
    coroutine_executor->options=E->options;
    bytecode_environment_init(&coroutine_executor->bytecode_environment);

    // coroutine shares garbage collector with owner
    coroutine_executor->gc=E->gc;
    vector_init(&coroutine_executor->traceback, sizeof(TracebackPoint), 16);

    // create a coroutine scope inheriting from global scope
    table_init(coroutine_executor, &coroutine_executor->bytecode_environment.scope);
    inherit_scope(coroutine_executor, coroutine_executor->bytecode_environment.scope, get(E, E->bytecode_environment.scope, to_string("global")));

    coroutine_executor->coroutine=coroutine.co;
    coroutine.co->state=co_uninitialized;

    // pass arguments and move to given function but don't call it yet
    REQUIRE_TYPE(function, t_function)
    REQUIRE(function.fp->ftype==f_bytecode, function)
    coroutine_executor->bytecode_environment.executed_program=(BytecodeProgram*)function.fp->source_pointer;
    heap_object_reference((HeapObject*)function.fp->source_pointer);
    REQUIRE(function.fp->arguments_count==arguments_count-1, function)
    for(int i=1; i<arguments_count; i++){
        push(&coroutine_executor->bytecode_environment.object_stack, arguments[i]);
    }

    coroutine.co->executor=coroutine_executor;
    return coroutine;
}

Object call_coroutine(Executor* E, Coroutine* coroutine, Object* arguments, int arguments_count){
    switch(coroutine->state){
        case co_uninitialized:
            if(arguments_count!=0){
                RETURN_ERROR("COROUTINE_ERROR", wrap_heap_object((HeapObject*)coroutine), "First call to coroutine shouldn't have any arguments, %i given", arguments_count)
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
                RETURN_ERROR("COROUTINE_ERROR", wrap_heap_object((HeapObject*)coroutine), "Coroutines can accept either zero or one argument, %i given", arguments_count)
            }
            return execute_bytecode(coroutine->executor);
        case co_finished: return null_const;
        default: RETURN_ERROR("COROUTINE_ERROR", wrap_heap_object((HeapObject*)coroutine), "Unknown coroutine state: %i", coroutine->state)
    }
}

Object builtin_print(Executor* E, Object scope, Object* arguments, int arguments_count){
    USING_STRING(cast(E, arguments[0], t_string).text,
        printf("%s\n", str));
    return null_const;
}

Object builtin_output(Executor* E, Object scope, Object* arguments, int arguments_count){
    USING_STRING(cast(E, arguments[0], t_string).text,
        printf("%s", str));
    return null_const;
}

Object builtin_input(Executor* E, Object scope, Object* arguments, int arguments_count){
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

Object builtin_substring(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    Object start=arguments[1];
    REQUIRE_TYPE(start, t_int)
    Object end=arguments[2];
    REQUIRE_TYPE(end, t_int)
    REQUIRE(start.int_value<end.int_value, multiple_causes(E, (Object[]){str, start, end}, 3))
    REQUIRE(start.int_value>=0, multiple_causes(E, (Object[]){str, start}, 2))
    REQUIRE(end.int_value<=strlen(str.text), multiple_causes(E, (Object[]){str, end}, 2))
    int length=end.int_value-start.int_value;
    char* result=malloc(sizeof(char)*(length+1));
    memcpy(result, str.text+(int)start.int_value, length);
    result[length]='\0';
    return to_string(result);
}

Object builtin_string_length(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    return to_int(strlen(str.text));
}

Object builtin_to_character(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object n=arguments[0];
    REQUIRE_TYPE(n, t_int);
    char* result=malloc(sizeof(char)*2);
    result[0]=n.int_value;
    result[1]='\0';
    return to_string(result);
}

Object builtin_from_character(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    REQUIRE(strlen(str.text)==1, str)
    return to_int(str.text[0]);
}

Object builtin_call(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object function=arguments[0];
    Object arguments_table=arguments[1];
    int i=0;
    Object object_at_i;
    do{
        object_at_i=get(E, arguments_table, to_int(i));
        i++;
    } while(object_at_i.type!=t_null);
    Object* arguments_array=malloc(sizeof(Object)*i);
    for(int j=0; j<i; j++){
        arguments_array[j]=get(E, arguments_table, to_int(j));
    }
    Object result=call(E, function, arguments_array, i);
    free(arguments_array);
    return result;
}

bool string_compare_parts(char* a, char* b, int length){
    for(int i=0; i<length; i++) {
        if(a[i]!=b[i]) {
            return false;
        }
    }
    return true;
}

Object builtin_format(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object str=arguments[0];
    REQUIRE_TYPE(str, t_string)
    stream s;
    stream_init(&s, 64);
    int variadic_counter=0;
    int str_length=strlen(str.text);

    #define NEXT_OBJECT \
        variadic_counter++; \
        if(variadic_counter>=arguments_count){ \
            RETURN_ERROR("FORMATTING_ERROR", multiple_causes(E, arguments, arguments_count), "Not enough arguments provided to format function."); \
        }

    for(int i=0; i<str_length; i++){
        #define COUNT_STR(s) (sizeof(s)/sizeof(char)-1)
        #define MATCH(s) (i+str_length>=COUNT_STR(s) && string_compare_parts(str.text+i, s, COUNT_STR(s)))
        if(MATCH("{}")){
            NEXT_OBJECT
            USING_STRING(cast(E, arguments[variadic_counter], t_string).text,
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

Object builtin_printf(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object to_print=builtin_format(E, scope, arguments, arguments_count);
    REQUIRE_TYPE(to_print, t_string)
    printf("%s\n", to_print.text);
    return null_const;
}

Object builtin_assert(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object tested=arguments[0];
    if(is_falsy(tested)){
        RETURN_ERROR("ASSERTION_FAILED", tested, "Assertion failed, the object is falsy.");
    } else {
        return null_const;
    }
}

Object builtin_assert_equal(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object a=arguments[0];
    Object b=arguments[1];
    if(compare(E, a, b)!=0){
        RETURN_ERROR("ASSERTION_FAILED", multiple_causes(E, (Object[]){a, b}, 2), "Assertion failed, provided objects are not equal.");
    } else {
        return null_const;
    }
}

Object builtin_typeof(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object type_name;
    string_init(&type_name);
    type_name.text=strdup(OBJECT_TYPE_NAMES[self.type]);
    return type_name;
}

Object builtin_table_get(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table)
    Object key=arguments[1];
    return table_get(E, self.tp, key);
}

Object builtin_table_set(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table)
    Object key=arguments[1]; 
    Object value=arguments[2];
    if(table_is_protected(self.tp)){
        table_set(E, self.tp, key, value);
        return value;
    } else {
        RETURN_ERROR("SET_ERROR", multiple_causes(E, (Object[]){self, key, value}, 3), "Function table_set was called on protected table.")
    }
}

Object builtin_disable_special_fields(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object table=arguments[0];
    REQUIRE_ARGUMENT_TYPE(table, t_table)
    table_disable_special_fields(table.tp);
    return table;
}

Object builtin_table_stringify(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object table=arguments[0];
    REQUIRE_ARGUMENT_TYPE(table, t_table)
    return to_string(table_stringify(E, table.tp));
}

Object builtin_table_copy(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object table=arguments[0];
    REQUIRE_ARGUMENT_TYPE(table, t_table)
    return table_copy(E, table.tp);
}

Object builtin_serialize(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    return to_string(serialize(E, self));
}

Object builtin_to_string(Executor* E, Object scope, Object* arguments, int arguments_count){
    return cast(E, arguments[0], t_string);
}

Object builtin_to_float(Executor* E, Object scope, Object* arguments, int arguments_count){
    return cast(E, arguments[0], t_float);
}

Object builtin_to_int(Executor* E, Object scope, Object* arguments, int arguments_count){
    return cast(E, arguments[0], t_int);
}

Object builtin_cast(Executor* E, Object scope, Object* arguments, int arguments_count){
    REQUIRE_TYPE(arguments[1], t_string);
    for(int i=0; i<OBJECT_TYPE_NAMES_COUNT; i++){
        if(strcmp(OBJECT_TYPE_NAMES[i], arguments[1].text)==0){
            return cast(E, arguments[0], (ObjectType)i);
        }
    }
    RETURN_ERROR("INCORRECT_ARGUMENT", arguments[1], "Incorrect type name");
}

Object builtin_iterator(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object o=arguments[0];
    return get_iterator(E, o);
}

Object builtin_include(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object path=arguments[0];
    Object result;
    REQUIRE_TYPE(path, t_string)
    Object sub_scope;
    table_init(E, &sub_scope);
    inherit_scope(E, sub_scope, E->scope);
    result=evaluate_file(E, path.text, sub_scope);
    return result;
}

Object builtin_eval(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object text=arguments[0];
    Object result;
    REQUIRE_TYPE(text, t_string)
    Object sub_scope;
    table_init(E, &sub_scope);
    inherit_scope(E, sub_scope, E->bytecode_environment.scope);
    result=evaluate_string(E, text.text, sub_scope);
    return result;
}

Object file_destroy(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table)
    Object pointer=table_get(E, self.tp, to_int(0));
    REQUIRE_TYPE(pointer, t_pointer)
    fclose((FILE*)pointer.p);
    return null_const;
}

Object builtin_time(Executor* E, Object scope, Object* arguments, int arguments_count){
    return to_int(time(NULL));
}

Object file_iterator_next(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object iterated=get(E, self, to_string("iterated"));
    REQUIRE_ARGUMENT_TYPE(iterated, t_table)
    Object pointer=table_get(E, iterated.tp, to_int(0));
    REQUIRE_TYPE(pointer, t_pointer)
    Object line_number=get(E, self, to_string("line_number"));
    REQUIRE_TYPE(line_number, t_int)
    set(E, self, to_string("line_number"), to_int(line_number.int_value+1));

    Object result;
    table_init(E, &result);
    set(E, result, to_string("key"), to_int(line_number.int_value));
    
    char* buffer=malloc(255);
    if(fgets_no_newline(buffer, 255, (FILE*)pointer.p)){
        set(E, result, to_string("value"), to_string(buffer));
        return result;
    } else {
        set(E, result, to_string("finished"), to_int(1));
        return result;
    }
}

Object file_iterator(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table)
    Object iterator;
    table_init(E, &iterator);
    set(E, iterator, to_string("iterated"), self);
    set(E, iterator, to_string("line_number"), to_int(0));
    set_function(E, iterator, "call", 1, false, file_iterator_next);
    Object pointer=table_get(E, self.tp, to_int(0));
    REQUIRE_TYPE(pointer, t_pointer)
    fseek((FILE*)pointer.p, 0, SEEK_SET);
    return iterator;
}

Object file_read_line(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table)
    Object pointer=table_get(E, self.tp, to_int(0));
    REQUIRE_TYPE(pointer, t_pointer)
    char* buffer=malloc(255);
    if(fgets_no_newline(buffer, 255, (FILE*)pointer.p)){
        return to_string(buffer);
    } else {
        free(buffer);
        return to_string("");
    }
}

Object file_read_entire(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table)
    Object pointer=table_get(E, self.tp, to_int(0));
    REQUIRE_TYPE(pointer, t_pointer)
    FILE* fp=(FILE*)pointer.p;
    char* content=read_entire_file(fp);

    if(content!=NULL){
        return to_string(content);
    } else {
        return to_string("");
    }
}

Object file_write(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table)
    Object text=arguments[1];
    REQUIRE_ARGUMENT_TYPE(text, t_string)
    Object pointer=table_get(E, self.tp, to_int(0));
    REQUIRE_TYPE(pointer, t_pointer)
    fputs(text.text, (FILE*)pointer.p);
    return text;
}

/*
How to make a safe object interacting with C pointers:
- Create an empty table.
- Use to_pointer function on the pointer and save it as one of table's fields.
- Use set_function_bound to set the table as function's enclosing scope.
- Use BOUND_FUNCTION_CHECK macro on top of all table's functions to check if first argument is same as the original table
- Create destroy function that frees the pointer.
- Call table_protect on the table.
- You can provide your own set override if you want to allow some of the fields being set.
*/

Object builtin_open_file(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object filename=arguments[0];
    REQUIRE_TYPE(filename, t_string)
    Object mode=arguments[1];
    REQUIRE_TYPE(mode, t_string)

    FILE* fp=fopen(filename.text, mode.text);
    if(!fp){
        RETURN_ERROR("FILE_ERROR", multiple_causes(E, (Object[]){filename, mode}, 2), "Opening file failed");
    }
    Object result;
    table_init(E, &result);
    set(E, result, to_int(0), to_pointer(fp));
    set_function_bound(E, result, "read_entire", 1, false, file_read_entire);
    set_function_bound(E, result, "read_line", 1, false, file_read_line);
    set_function_bound(E, result, "write", 2, false, file_write);
    set_function_bound(E, result, "iterator", 2, false, file_iterator);
    set_function_bound(E, result, "destroy", 1, false, file_destroy);
    table_protect(result.tp);

    return result;
}

Object builtin_remove_file(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object filename=arguments[0];
    REQUIRE_TYPE(filename, t_string)
    remove(filename.text);
    return null_const;
}

Object builtin_import_dll(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object filename=arguments[0];
    REQUIRE_TYPE(filename, t_string)
    return import_dll(E, filename.text);
}

Object builtin_exit(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object execution_result=arguments[0];
    USING_STRING(stringify(E, execution_result), 
        printf("The script \"%s\" has exited with result:\n%s\n", E->file, str));
    exit(0);
}

Object builtin_terminate(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object exit_code=arguments[0];
    REQUIRE_TYPE(exit_code, t_int);
    printf("The script \"%s\" has terminated with return code %i", E->file, exit_code.int_value);
    exit(exit_code.int_value);
}

Object builtin_traceback(Executor* E, Object scope, Object* arguments, int arguments_count){
    printf("traceback:\n");
    for(int i=0; i<vector_count(&E->traceback); i++){
        TracebackPoint* traceback_point=(TracebackPoint*)vector_index(&E->traceback, i);
        char* line=get_source_line(traceback_point->file_name, traceback_point->line_number);
        printf("%s:%i %s\n", traceback_point->file_name, traceback_point->line_number, line);
        free(line);
    }
    return null_const;
}

Object builtin_error(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object type=arguments[0];
    REQUIRE_ARGUMENT_TYPE(type, t_string)
    Object cause=arguments[1];
    Object message=arguments[2];
    REQUIRE_ARGUMENT_TYPE(message, t_string)
    Object error;
    NEW_ERROR(error, type.text, cause, message.text)
    return error;
}

Object builtin_copy(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object to_copy=arguments[0];
    return copy(E, to_copy);
}

Object builtin_set_random_seed(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object number=arguments[0];
    REQUIRE_ARGUMENT_TYPE(number, t_int)
    srand(number.int_value);
    return null_const;
}

Object builtin_create_variant(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object function=arguments[0];
    REQUIRE_ARGUMENT_TYPE(function, t_function)
    if(function.fp->ftype!=f_bytecode){
        RETURN_ERROR("WRONG_ARGUMENT_TYPE", function, "Function passed to create_variant isn't a bytecode function");
    }
    create_variant(E, (BytecodeProgram*)function.fp->source_pointer);
    return null_const;
}

Object builtin_random_int(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object left=arguments[0];
    Object right=arguments[1];
    REQUIRE_ARGUMENT_TYPE(left, t_int)
    REQUIRE_ARGUMENT_TYPE(right, t_int)
    int interval=right.int_value-left.int_value;
    int random_in_interval=rand()%interval;
    return to_int(left.int_value+random_in_interval);
}

Object builtin_random_01(Executor* E, Object scope, Object* arguments, int arguments_count){
    return to_float((float)rand()/RAND_MAX);
}

Object builtin_multiple_causes(Executor* E, Object scope, Object* arguments, int arguments_count){
    return multiple_causes(E, arguments, arguments_count);
}

Object builtins_table(Executor* E){
    Object scope;
    table_init(E, &scope);
    set(E, scope, to_int(0), to_string("builtins_table"));
    set(E, scope, to_string("builtins"), scope);
    #define REGISTER(f, args_count) \
        Object f##_function; \
        function_init(E, &f##_function); \
        f##_function.fp->arguments_count=args_count; \
        f##_function.fp->native_pointer=&builtin_##f; \
        set(E, scope, to_string(#f), f##_function);
    REGISTER(print, 1)
    REGISTER(output, 1)
    REGISTER(input, 0)
    REGISTER(assert, 1)
    REGISTER(assert_equal, 2)
    REGISTER(typeof, 1)
    REGISTER(table_get, 2)
    REGISTER(table_set, 3)
    REGISTER(table_stringify, 1)
    REGISTER(table_copy, 1)
    REGISTER(disable_special_fields, 1)
    REGISTER(include, 1)
    REGISTER(eval, 1)
    REGISTER(substring, 3)
    REGISTER(string_length, 1)
    REGISTER(from_character, 1)
    REGISTER(to_character, 1)
    REGISTER(to_string, 1)
    REGISTER(to_float, 1)
    REGISTER(to_int, 1)
    REGISTER(cast, 2)
    REGISTER(open_file, 2)
    REGISTER(remove_file, 1)
    REGISTER(import_dll, 1)
    REGISTER(iterator, 1)
    REGISTER(time, 0)
    REGISTER(exit, 1)
    REGISTER(terminate, 1)
    REGISTER(traceback, 0)
    REGISTER(error, 3)
    REGISTER(copy, 1)
    REGISTER(set_random_seed, 1)
    REGISTER(random_int, 2)
    REGISTER(random_01, 0)
    REGISTER(call, 2)
    REGISTER(create_variant, 1)
    REGISTER(serialize, 1)
    #undef REGISTER

    Object yield;
    function_init(E, &yield);
    yield.fp->ftype=f_special;
    yield.fp->special_index=0;
    yield.fp->variadic=true;
    set(E, scope, to_string("yield"), yield);

    Object debug;
    function_init(E, &debug);
    debug.fp->ftype=f_special;
    debug.fp->special_index=1;
    set(E, scope, to_string("debug"), debug);

    Object collect_garbage;
    function_init(E, &collect_garbage);
    collect_garbage.fp->ftype=f_special;
    collect_garbage.fp->special_index=2;
    set(E, scope, to_string("collect_garbage"), collect_garbage);

    set_function(E, scope, "table_iterator", 1, false, table_get_iterator_object);

    set_function(E, scope, "coroutine", 1, true, builtin_coroutine);
    set_function(E, scope, "format", 1, true, builtin_format);
    set_function(E, scope, "printf", 1, true, builtin_printf);
    set_function(E, scope, "multiple_causes", 0, true, builtin_multiple_causes);

    return scope;
}

Object scope_get_override(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    Object key=arguments[1];

    Object base=self;
    Object map_get_result=table_get(E, self.tp, key);

    // we assume that all scopes are of type Table and have same get behaviour
    while(map_get_result.type==t_null){
        base=table_get(E, base.tp, to_string("base"));
        if(base.type!=t_table){
            return null_const;
        } else {
            map_get_result=table_get(E, base.tp, key);
        }
    }
    return map_get_result;
}

Object scope_set_override(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    Object key=arguments[1];
    Object value=arguments[2];

    if(EQUALS_STRING(key, "set")){
        key=to_string("_set");
    }

    Object checked=self;
    Object checked_zero_index=null_const;
    Object map_get_result=null_const;

    do{
        map_get_result=table_get(E, checked.tp, key);
        if(map_get_result.type!=t_null){
            // key was found in outer scope
            table_set(E, checked.tp, key, value);

            // destroy_unreferenced(E, &checked);
            destroy_unreferenced(E, &checked_zero_index);
            return value;
        } else {
            // destroy_unreferenced(E, &checked);
            destroy_unreferenced(E, &checked_zero_index);
            checked=table_get(E, checked.tp, to_string("base"));
            checked_zero_index=table_get(E, checked.tp, to_int(0));
        }
    } while(checked.type==t_table 
         && !EQUALS_STRING(checked_zero_index, "builtins_table"));// builtins table can't be changed
    // key wasn't found in any outer scope
    table_set(E, self.tp, key, value);
    
    // destroy_unreferenced(E, &checked);
    destroy_unreferenced(E, &checked_zero_index);
    return value;
}

void inherit_scope(Executor* E, Object scope, Object base){
    if(scope.type!=t_table){
        return;
    }
    Object base_global=table_get(E, base.tp, to_string("global"));
    if(base_global.type!=t_null){
        table_set(E, scope.tp, to_string("global"), base_global);
    } else {
        Object base_zero_index=table_get(E, base.tp, to_int(0));
        if(EQUALS_STRING(base_zero_index, "builtins_table")){
            table_set(E, scope.tp, to_string("global"), scope);
        } else {
            table_set(E, scope.tp, to_string("global"), base);
        }
        destroy_unreferenced(E, &base_zero_index);
    }
    table_set(E, scope.tp, to_string("base"), base);
    set_function(E, scope, "get", 2, false, scope_get_override);
    set_function(E, scope, "set", 3, false, scope_set_override);
}