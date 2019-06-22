#include "execution.h"

Object evaluate(Executor* E, expression* parsing_result, Object scope, bool delete_ast){
    if(parsing_result==NULL){
        return null_const;// there was an error while parsing
    }
    execute_macros(E, parsing_result);
    if(E->options.optimise_ast){
        optimise_ast(E, parsing_result);
    }
    if(E->options.print_ast){
        USING_STRING(stringify_expression(parsing_result, 0),
            printf("Abstract Syntax Tree:\n%s\n", str));
    }
    Object execution_result;
    if(E->options.ast_only){
        reference(&scope);
        execution_result=execute_ast(E, parsing_result, scope, 1);
        if(delete_ast) delete_expression(parsing_result);
    } else {
        BytecodeProgram prog=ast_to_bytecode(parsing_result, true);
        if(delete_ast) delete_expression(parsing_result);// at this point ast is useless and only wastes memory
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
    return evaluate(E, parsing_result, scope, true);
}

Object evaluate_file(Executor* E, const char* file_name, Object scope){
    expression* parsing_result=parse_file(file_name);
    E->file=file_name;
    return evaluate(E, parsing_result, scope, true);
}

void execute_file(Executor* E, const char* file_name){
    Object global_scope;
    table_init(E, &global_scope);
    reference(&global_scope);
    register_builtins(E, global_scope);
    Object execution_result=evaluate_file(E, file_name, global_scope);
    USING_STRING(stringify(E, execution_result), 
        printf("Execution result:\n%s\n", str));
    USING_STRING(stringify(E, global_scope), 
       printf("Global scope:\n%s\n", str));
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

GarbageCollector* get_garbage_collector(Executor* E){
    return E->gc;
}