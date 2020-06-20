#include "builtins.h"

Object builtin_coroutine(Executor* E, Object scope, Object* arguments, int arguments_count){
    return new_coroutine(E, arguments[0], arguments+1, arguments_count-1);
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
    #define INPUT_COUNT 128
    char* input=malloc(INPUT_COUNT*sizeof(char));
    if(fgets_no_newline(input, INPUT_COUNT, stdin)!=NULL){
        return to_string(input);
    } else {
        return null_const;
    }
    #undef INPUT_COUNT
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
            stream_deinit(&s); \
            RETURN_ERROR("FORMATTING_ERROR", multiple_causes(E, arguments, arguments_count), "Not enough arguments provided to format function."); \
        }

    for(int i=0; i<str_length; i++){
        #define COUNT_STR(s) (sizeof(s)/sizeof(char)-1)
        #define MATCH(s) (i+str_length>=COUNT_STR(s) && string_compare_parts(str.text+i, s, COUNT_STR(s)))
        #define WRITE_NEXT_VARIADIC_ARGUMENT \
            NEXT_OBJECT \
            if(arguments[variadic_counter].type==t_string){ \
                stream_push_string(&s, arguments[variadic_counter].text); \
            } else { \
                USING_STRING(stringify(E, arguments[variadic_counter]), \
                    stream_push_string(&s, str)) \
            }
        if(MATCH("{}")){
            WRITE_NEXT_VARIADIC_ARGUMENT
            i++;
        } else if(MATCH("\\{}")) {
            stream_push(&s, &str.text[i+1], sizeof(char)*2);
            i+=2;
        } else if(MATCH("\\\\{}")){
            stream_push(&s, "\\", sizeof(char));
            WRITE_NEXT_VARIADIC_ARGUMENT
            i+=3;
        } else {
            stream_push(&s, &str.text[i], sizeof(char));
        }
        #undef COUNT_STR
        #undef MATCH
        #undef WRITE_NEXT_VARIADIC_ARGUMENT
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

Object builtin_assert_error(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object tested=arguments[0];
    if(is_error(E, tested)){
        error_handle(E, tested);
        return null_const;
    } else {
        RETURN_ERROR("ASSERTION_FAILED", tested, "Assertion failed, the object isn't an error.");
    }
}

Object builtin_assert_equal(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object a=arguments[0];
    Object b=arguments[1];
    Object error=null_const;
    if(compare_and_get_error(E, a, b, &error)!=0){
        Object causes;
        if(error.type!=t_null){
            causes=multiple_causes(E, OBJECTS_ARRAY(a, b, error), 3);
        } else {
            causes=multiple_causes(E, OBJECTS_ARRAY(a, b), 2);
        }
        RETURN_ERROR("ASSERTION_FAILED", causes, "Assertion failed, objects aren't equal.");
    } else {
        return null_const;
    }
}

Object builtin_get_type(Executor* E, Object scope, Object* arguments, int arguments_count){
    return get_type_symbol(E, arguments[0].type);
}

Object builtin_get_type_name(Executor* E, Object scope, Object* arguments, int arguments_count){
    return to_string(strdup(get_type_name(arguments[0].type)));
}

Object builtin_table_get(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table)
    Object key=arguments[1];
    Object result=table_get(E, self.tp, key);
    reference(&result);
    return result;
}

Object builtin_collect_garbage(Executor* E, Object scope, Object* arguments, int arguments_count){
    executor_collect_garbage(E);
    return null_const;
}

Object builtin_debug(Executor* E, Object scope, Object* arguments, int arguments_count){
    if(E->options.debug){
        E->debugger.running=false;
        debugger_update(E, &E->debugger);
    }
    return null_const;
}

Object builtin_table_set(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table)
    Object key=arguments[1]; 
    Object value=arguments[2];
    if(table_is_protected(self.tp)){
        table_set(E, self.tp, key, value);
        reference(&value);
        return value;
    } else {
        RETURN_ERROR("SET_ERROR", multiple_causes(E, (Object[]){self, key, value}, 3), "Function table_set was called on protected table.")
    }
}

Object builtin_protect(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object table=arguments[0];
    REQUIRE_ARGUMENT_TYPE(table, t_table)
    table_protect(table.tp);
    return table;
}

Object builtin_table_stringify(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object table=arguments[0];
    REQUIRE_ARGUMENT_TYPE(table, t_table)
    return to_string(table_stringify(E, table.tp));
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
    REQUIRE_TYPE(arguments[1], t_symbol);
    for(int i=0; i<=LAST_OBJECT_TYPE; i++){
        if(compare(E, get_type_symbol(E, i), arguments[1])==0){
            return cast(E, arguments[0], i);
        }
    }
    RETURN_ERROR("INCORRECT_ARGUMENT", arguments[1], "Incorrect type symbol.");
}

Object builtin_is_error(Executor* E, Object scope, Object* arguments, int arguments_count){
    return to_int(is_error(E, arguments[0]));
}

Object builtin_iterator(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object o=arguments[0];
    return get_iterator(E, o);
}

Object builtin_import(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object path=arguments[0];
    Object result;
    REQUIRE_TYPE(path, t_string)
    Object sub_scope;
    table_init(E, &sub_scope);
    inherit_global_scope(E, sub_scope.tp);
    result=evaluate_file(E, path.text, sub_scope);
    dereference(E, &sub_scope);
    return result;
}

Object builtin_evaluate(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object text=arguments[0];
    REQUIRE_TYPE(text, t_string)
    Object sub_scope;
    table_init(E, &sub_scope);
    inherit_global_scope(E, sub_scope.tp);
    Object result=evaluate_string(E, text.text, sub_scope);
    dereference(E, &sub_scope);
    return result;
}

Object builtin_evaluate_expression(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object expression=arguments[0];
    Expression* converted=object_to_expression(E, expression);
    Object sub_scope;
    table_init(E, &sub_scope);
    inherit_global_scope(E, sub_scope.tp);
    Object result=evaluate(E, converted, sub_scope, "evaluate_expression", true);
    dereference(E, &sub_scope);
    return result;
}

Object builtin_parse(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object text=arguments[0];
    REQUIRE_TYPE(text, t_string)
    Expression* parsing_result=parse_string(text.text);
    Object result=expression_to_object(E, parsing_result);
    delete_expression(parsing_result);
    return result;
}

Object file_destroy(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table)
    Object pointer=table_get(E, self.tp, to_int(0));
    REQUIRE_TYPE(pointer, t_pointer)
    fclose((FILE*)pointer.p);
    return null_const;
}

Object builtin_sleep(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object interval=arguments[0];
    REQUIRE_ARGUMENT_TYPE(interval, t_int)
    time_t after_interval=time(NULL)+interval.int_value;
    while(time(NULL)<=after_interval);
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
    table_set(E, self.tp, to_string("line_number"), to_int(line_number.int_value+1));

    Object result;
    table_init(E, &result);
    table_set(E, result.tp, to_string("key"), to_int(line_number.int_value));
    
    char* buffer=malloc(255);
    if(fgets_no_newline(buffer, 255, (FILE*)pointer.p)){
        table_set(E, result.tp, to_string("value"), to_string(buffer));
        return result;
    } else {
        table_set(E, result.tp, to_string("finished"), to_int(1));
        return result;
    }
}

Object file_iterator(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table)
    Object iterator;
    table_init(E, &iterator);
    table_set(E, iterator.tp, to_string("iterated"), self);
    table_set(E, iterator.tp, to_string("line_number"), to_int(0));
    Object next_function=to_bound_function(E, iterator, 1, false, file_iterator_next);
    table_set(E, iterator.tp, to_string("next"), next_function);
    dereference(E, &next_function);
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
    table_set(E, result.tp, to_int(0), to_pointer(fp));
    set_function_bound(E, result, to_string("read_entire"), 1, false, file_read_entire);
    set_function_bound(E, result, to_string("read_line"), 1, false, file_read_line);
    set_function_bound(E, result, to_string("write"), 2, false, file_write);
    set_function_bound(E, result, OVERRIDE(E, iterator), 2, false, file_iterator);
    set_function_bound(E, result, OVERRIDE(E, destroy), 1, false, file_destroy);
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
    if(E->options.print_result){
        USING_STRING(stringify(E, execution_result), 
            printf("The script \"%s\" has exited with result:\n%s\n", E->file, str));
    }
    exit(0);
}

Object builtin_terminate(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object exit_code=arguments[0];
    REQUIRE_TYPE(exit_code, t_int);
    if(E->options.print_result){
        printf("The script \"%s\" has terminated with return code %i\n", E->file, exit_code.int_value);
    }
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

Object builtin_new_error(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object type=arguments[0];
    REQUIRE_ARGUMENT_TYPE(type, t_string)
    Object cause=arguments[1];
    Object message=arguments[2];
    REQUIRE_ARGUMENT_TYPE(message, t_string)
    Object error;
    NEW_ERROR(error, type.text, cause, "%s", message.text)
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

Object builtin_new_symbol(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object comment=arguments[0];
    REQUIRE_ARGUMENT_TYPE(comment, t_string)
    return new_symbol(E, comment.text);
}

Object match(Executor* E, Object tested, Object signature){
    switch(signature.type){
        case t_symbol:
            return to_int(get_type_symbol(E, tested.type).sp->index==signature.sp->index);
        case t_function:
            return call(E, signature, &tested, 1);
        case t_table: {
            Object it;
            FOREACH(signature, it, 
                Object key=get(E, it, to_string("key"));
                Object value=get(E, it, to_string("value"));
                Object at_key=get(E, tested, key);
                if(is_error(E, at_key)){
                    dereference(E, &key);
                    dereference(E, &value);
                    error_handle(E, at_key);
                    dereference(E, &at_key);
                    return to_int(false);
                }
                Object match_result=match(E, at_key, value);
                if(is_falsy(match_result)) {
                    return match_result;
                }
            )
            return to_int(1);
        }
        default: return to_int(compare(E, tested, signature)==0);
    }
}

Object builtin_match(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object tested=arguments[0];
    Object signature=arguments[1];
    return match(E, tested, signature);
}

Object builtin_map(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object iterable=arguments[0];
    Object function=arguments[1];
    Object it;
    Object result;
    table_init(E, &result);
    FOREACH(iterable, it, {
        Object key=get(E, it, to_string("key"));
        Object value=get(E, it, to_string("value"));
        Object passed_value=call(E, function, &value, 1);
        table_set(E, result.tp, key, passed_value);
        dereference(E, &key);
        dereference(E, &value);
        dereference(E, &passed_value);
    })
    return result;
}

/*
help(subject)

Displays help related to subject.
*/
Object builtin_help(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object subject=arguments[0];
    return help(E, subject);
}

char** copy_arguments(int arguments_count, ...){
    if(arguments_count==0)
        return NULL;
    char** result=malloc(sizeof(char*)*arguments_count);
    va_list va;
    va_start (va, arguments_count);
    for(int i=0; i<arguments_count; i++){
        result[i]=strdup(va_arg(va, char*));
    }
    va_end (va);
    return result;
}

Object builtins_table(Executor* E){
    Object scope;
    table_init(E, &scope);

    table_set(E, scope.tp, to_int(0), to_string("builtins"));
    table_set(E, scope.tp, to_string("builtins"), scope);
    table_set(E, scope.tp, to_string("overrides"), E->object_system.overrides_table);
    table_set(E, scope.tp, to_string("types"), E->object_system.types_table);
    table_set(E, scope.tp, to_string("undefined_argument"), E->undefined_argument);

    #define REGISTER_(with_help, f, hlp, args_count, ...) \
    { \
        Object function; \
        function_init(E, &function); \
        function.fp->arguments_count=args_count; \
        function.fp->native_pointer=&builtin_##f; \
        function.fp->name=strdup(#f); \
        if(with_help){ \
            function.fp->help=strdup(hlp); \
            function.fp->argument_names=copy_arguments(args_count, ##__VA_ARGS__); \
        } \
        table_set(E, scope.tp, to_string(#f), function); \
        dereference(E, &function);  \
    }

    #define REGISTER_WITH_HELP(f, hlp, args_count, ...) REGISTER_(true, f, hlp, args_count, ##__VA_ARGS__)

    #define REGISTER(f, args_count) REGISTER_(false, f, "", args_count)

    REGISTER_WITH_HELP(print, "Outputs a text into the console and inserts newline after it.", 1, "text")
    REGISTER_WITH_HELP(output, "Outputs a text into the console.", 1, "text")
    REGISTER_WITH_HELP(input, "Returns the string written into the console by the user.", 0)
    REGISTER_WITH_HELP(assert, "Fails if assertion is falsy", 1, "assertion")
    REGISTER_WITH_HELP(assert_equal, "Fails if actual is different than expected.", 2, "actual", "expected")
    REGISTER_WITH_HELP(assert_error, "Fails if expected_error isn't an error object.", 1, "expected_error")
    REGISTER_WITH_HELP(get_type, "Returns type of the object.", 1, "object")
    REGISTER_WITH_HELP(get_type_name, "Returns name of the type of the object.", 1, "object")
    REGISTER(table_get, 2)
    REGISTER(table_set, 3)
    REGISTER(table_stringify, 1)
    REGISTER(protect, 1)
    REGISTER(import, 1)
    REGISTER(evaluate, 1)
    REGISTER(substring, 3)
    REGISTER(string_length, 1)
    REGISTER(from_character, 1)
    REGISTER(to_character, 1)
    REGISTER(to_string, 1)
    REGISTER(to_float, 1)
    REGISTER(to_int, 1)
    REGISTER_WITH_HELP(cast, "Casts object to the type.", 2, "object", "type")
    REGISTER_WITH_HELP(open_file, 
    "Opens a file in a specified mode and returns its object.\n"
    "Refer to C's fopen function for available modes.\n"
    "Call help on file object to get more help."
    , 2, "file_name", "mode")
    REGISTER(remove_file, 1)
    REGISTER(import_dll, 1)
    REGISTER(iterator, 1)
    REGISTER(sleep, 1)
    REGISTER(time, 0)
    REGISTER(exit, 1)
    REGISTER(terminate, 1)
    REGISTER(traceback, 0)
    REGISTER_WITH_HELP(new_error, "Creates a new error object.", 3, "type", "cause", "message")
    REGISTER(copy, 1)
    REGISTER(set_random_seed, 1)
    REGISTER_WITH_HELP(random_int, "Returns a random number between start(inclusive) and end(exclusive).", 2, "start", "end")
    REGISTER(random_01, 0)
    REGISTER(call, 2)
    REGISTER(create_variant, 1)
    REGISTER(serialize, 1)
    REGISTER_WITH_HELP(new_symbol, "Creates a new symbol object.", 1, "comment")
    REGISTER(is_error, 1)
    REGISTER(collect_garbage, 0)
    REGISTER(debug, 0)
    REGISTER_WITH_HELP(match, 
    "If signature is a type it returns true if tested belongs to the type.\n"
    "If signature is a table it goes through its keys\n"
    "and for each key gets the value at the key from tested and signature and calls match on them.\n"
    "If match returned truthy value for each field then the result is true, else the result is false.\n"
    "If signature is a function it calls it with tested and returns the result.\n"
    "This property can be used to express more complex ideas like type alternative.", 
    2, "tested", "signature")
    REGISTER(evaluate_expression, 1)
    REGISTER(parse, 1)
    REGISTER_WITH_HELP(map, 
    "Returns a new table constructed by calling function on iterable fields' values.\n"
    "For example map([a=2, b=3], x->x*2) returns [a=4, b=6]."
    , 2, "iterable", "function")
    REGISTER_WITH_HELP(help, 
    "Shows help for the subject. Subject can be an object of any type\n."
    "Some of the builtin objects have their help set.\n"
    "To add help to a table set it's overrides.help field.\n"
    "For example: tab=[.[overrides.help]=\"Help for the object.\"].\n"
    "If the first line of function is a string literal its value is used as this function's help."
    , 1, "subject")
    #undef REGISTER

    Object function;
    function_init(E, &function);
    function.fp->ftype=f_special;
    function.fp->special_index=0;
    function.fp->arguments_count=0;
    function.fp->variadic=true;
    table_set(E, scope.tp, to_string("yield"), function);
    dereference(E, &function);
    
    set_function(E, scope, to_string("table_iterator"), 1, false, table_get_iterator_object);

    set_function(E, scope, to_string("coroutine"), 1, true, builtin_coroutine);
    set_function(E, scope, to_string("format"), 1, true, builtin_format);
    set_function(E, scope, to_string("printf"), 1, true, builtin_printf);
    set_function(E, scope, to_string("multiple_causes"), 0, true, builtin_multiple_causes);

    return scope;
}

Object scope_set_override(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    Object key=arguments[1];
    Object value=arguments[2];

    Object indexed=self;
    Object indexed_zero_index=null_const;
    Object map_get_result=null_const;

    do{
        map_get_result=table_get(E, indexed.tp, key);
        if(map_get_result.type!=t_null){
            // key was found in outer scope
            table_set(E, indexed.tp, key, value);

            reference(&value);
            return value;
        } else {
            indexed=table_get(E, indexed.tp, OVERRIDE(E, prototype));
            if(indexed.type==t_table){
                indexed_zero_index=table_get(E, indexed.tp, to_int(0));
            }
        }
    } while(indexed.type==t_table 
         && !EQUALS_STRING(indexed_zero_index, "builtins"));// builtins table can't be changed
    // key wasn't found in any outer scope
    table_set(E, self.tp, key, value);

    reference(&value);
    return value;
}

void inherit_scope(Executor* E, Table* scope, Object base){
    table_set(E, scope, OVERRIDE(E, prototype), base);
    table_set(E, scope, OVERRIDE(E, set), to_native_function(E, scope_set_override, NULL, 3, false));
}

void inherit_current_scope(Executor* E, Table* scope){
    inherit_scope(E, scope, E->scope);
}

void inherit_global_scope(Executor* E, Table* scope){
    Object global_scope=get(E, E->scope, to_string("builtins"));
    inherit_scope(E, scope, global_scope);
    dereference(E, &global_scope);
}