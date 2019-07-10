#include "execution.h"

Object evaluate(Executor* E, expression* parsing_result, Object scope, const char* file_name, bool delete_ast){
    if(parsing_result==NULL){
        return null_const;// there was an error while parsing
    }
    execute_macros(E, &parsing_result);
    if(E->options.optimise_ast){
        optimise_ast(E, &parsing_result);
    }
    if(E->options.print_ast){
        USING_STRING(stringify_expression(parsing_result, 0),
            printf("Abstract Syntax Tree:\n%s\n", str));
    }
    Object execution_result;
    if(E->options.ast_only){
        reference(&scope);
        execution_result=execute_ast(E, parsing_result, scope, 1);
        if(delete_ast){ 
            delete_expression(parsing_result);
        }
    } else {
        BytecodeProgram prog=ast_to_bytecode(parsing_result, true);
        prog.source_file_name=strdup(file_name);
        if(delete_ast){
            delete_expression(parsing_result);// at this point ast is useless and only wastes memory
        }
        if(E->options.optimise_bytecode){
            optimise_bytecode(E, &prog, E->options.print_bytecode_optimisations);
        }
        if(E->options.print_bytecode){
            USING_STRING(stringify_bytecode(&prog),
                printf("Bytecode:\n%s\n", str));
        }
        create_return_point(&E->bytecode_environment, true);
        E->bytecode_environment.pointer=0;
        E->bytecode_environment.executed_program=malloc(sizeof(BytecodeProgram));
        memcpy(E->bytecode_environment.executed_program, &prog, sizeof(BytecodeProgram));
        bytecode_program_init(E, E->bytecode_environment.executed_program);
        // the end instruction will dereference these later
        gc_object_reference((gc_Object*)E->bytecode_environment.executed_program);
        reference(&scope);
        E->bytecode_environment.scope=scope;
        
        execution_result=execute_bytecode(E);
    }
    return execution_result;
}

Object evaluate_string(Executor* E, const char* s, Object scope){
    expression* parsing_result=parse_string(s);
    E->file="string";
    return evaluate(E, parsing_result, scope, "string", true);
}

Object evaluate_file(Executor* E, const char* file_name, Object scope){
    expression* parsing_result=parse_file(file_name);
    E->file=file_name;
    return evaluate(E, parsing_result, scope, file_name, true);
}

static Object arguments_to_table(Executor* E, char** arguments){
    Object arguments_table;
    table_init(E, &arguments_table);
    int i=0;
    for(; arguments[i]!=NULL; i++){
        table_set(E, arguments_table.tp, to_int(i), to_string(arguments[i]));
    }
    table_set(E, arguments_table.tp, to_string("count"), to_int(i));
    return arguments_table;
}

void execute_file(Executor* E, const char* file_name, char** arguments){
    Object global_scope;
    table_init(E, &global_scope);
    reference(&global_scope);
    table_set(E, global_scope.tp, to_string("arguments"), arguments_to_table(E, arguments));
    if(E->options.include_builtins){
        register_builtins(E, global_scope);
    }
    Object execution_result=evaluate_file(E, file_name, global_scope);
    USING_STRING(stringify(E, execution_result), 
        printf("The script \"%s\" has exited with result:\n%s\n", file_name, str));
    dereference(E, &execution_result);
    dereference(E, &global_scope);
}

// this function should only be called from call_function, it's there to simplify the code structure
Object call_function_processed(Executor* E, Function* f, Object* arguments, int arguments_count){
    if(f->ftype==f_ast){
        Object function_scope;
        table_init(E, &function_scope);
        if(f->enclosing_scope.type!=t_null){
            inherit_scope(E, function_scope, f->enclosing_scope);
        }
        for(int i=0; i<arguments_count; i++){
            STRING_OBJECT(argument_name, f->argument_names[i]);
            set(E, function_scope, argument_name, arguments[i]);
        }
        return execute_ast(E, ((ASTSourcePointer*)f->source_pointer)->body, function_scope, 1);
    } else if(f->ftype==f_bytecode){
        create_return_point(&E->bytecode_environment, true);
        move_to_function(E, f);
        for(int i=0; i<arguments_count; i++){
            push(&E->bytecode_environment.object_stack, arguments[i]);
        }
        Object result=execute_bytecode(E);
        return result;
    } else {
        RETURN_ERROR("FUNCTION_TYPE_ERROR", wrap_gc_object((gc_Object*)f), "Function type has incorrect type value of %i", f->ftype);
        return null_const;
    }
}

// return error if arguments count is incorrect and proccess variadic functions, then call the Function using call_function_processed
Object call_function(Executor* E, Function* f, Object* arguments, int arguments_count){
    if(f->ftype==f_native){
        return f->native_pointer(E, arguments, arguments_count);
    } else {
        int arguments_count_difference=arguments_count-f->arguments_count;
        if(f->variadic){
            if(arguments_count_difference<-1){
                RETURN_ERROR("CALL_ERROR", null_const, "Not enough arguments in variadic function call, expected at least %i, given %i.", f->arguments_count-1, arguments_count);
            }
            // make new arguments array
            int processed_arguments_count=f->arguments_count;
            Object* processed_arguments=malloc(sizeof(Object)*processed_arguments_count);
            // copy non variadic arguments
            for(int i=0; i<f->arguments_count-1; i++){
                processed_arguments[i]=arguments[i];
            }
            // pack variadic arguments into a Table
            int variadic_arguments_count=arguments_count_difference+1;
            Object variadic_table;
            table_init(E, &variadic_table);
            for(int i=variadic_arguments_count-1; i>=0; i--){
                set(E, variadic_table, to_int(i), arguments[f->arguments_count-1+i]);
            }
            // append the variadic array to the end of processed arguments array
            processed_arguments[f->arguments_count-1]=variadic_table;

            return call_function_processed(E, f, processed_arguments, processed_arguments_count);
        } else if(arguments_count_difference<0){
            RETURN_ERROR("CALL_ERROR", null_const, "Not enough arguments in function call, expected %i, given %i.", f->arguments_count, arguments_count);
        } else if(arguments_count_difference>0) {
            RETURN_ERROR("CALL_ERROR", null_const, "Too many arguments in function call, expected %i, given %i.", f->arguments_count, arguments_count);
        } else {
            return call_function_processed(E, f, arguments, arguments_count);
        }
    }
}

GarbageCollector* executor_get_garbage_collector(Executor* E){
    return E->gc;
}

Object executor_on_unhandled_error(Executor* E, Object error) {
    if(E->scope.type==t_table){
        Object handler=get(E, E->scope, to_string("on_unhandled_error"));
        if(handler.type!=t_null){
            return call(E, handler, &error, 1);
        }
    }
    USING_STRING(stringify(E, error),
        printf("Unhandled error:\n%s", str));
    return null_const;
}

void executor_foreach_children(Executor* E, Executor* iterated_executor, gc_PointerForeachChildrenCallback callback){
    vector* object_stack=&E->bytecode_environment.object_stack;
    vector* return_stack=&E->bytecode_environment.return_stack;
    vector* used_objects=&E->ast_execution_state.used_objects;
    for(int i=0; i<vector_count(return_stack); i++){
        ReturnPoint* return_point=vector_index(return_stack, i);
        callback(E, &return_point->scope);
        Object wrapped_program=wrap_gc_object((gc_Object*)return_point->program);
        callback(E, &wrapped_program);
    }
    for(int i=0; i<vector_count(object_stack); i++){
        callback(E, (Object*)vector_index(object_stack, i));
    }
    for(int i=0; i<vector_count(used_objects); i++){
        callback(E, (Object*)vector_index(used_objects, i));
    }
    callback(E, &E->scope);
    Object wrapped_program=wrap_gc_object((gc_Object*)E->bytecode_environment.executed_program);
    callback(E, &wrapped_program);
}

void coroutine_foreach_children(Executor* E, Coroutine* co, gc_PointerForeachChildrenCallback callback){
    executor_foreach_children(E, co->executor, callback);
}

void coroutine_free(Coroutine* co){
    free(co->executor);
    free(co);
}

void executor_collect_garbage(Executor* E){
    gc_unmark_all(E->gc);
    executor_foreach_children(E, E, gc_mark);
    gc_sweep(E);
}

void executor_init(Executor* E){
    E->gc=malloc(sizeof(GarbageCollector));
    E->options=default_options;
    E->ast_execution_state.returning=false;
    E->scope=null_const;
    vector_init(&E->traceback, sizeof(TracebackPoint), 16);
    vector_init(&E->ast_execution_state.used_objects, sizeof(Object), 8);
    object_system_init(E);
    bytecode_environment_init(&E->bytecode_environment);
}

void executor_deinit(Executor* E){
    E->scope=null_const;
    object_system_deinit(E);
    vector_deinit(&E->traceback);
    vector_deinit(&E->ast_execution_state.used_objects);
    bytecode_environment_deinit(&E->bytecode_environment);
    free(E->gc);
}