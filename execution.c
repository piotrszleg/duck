#include "execution.h"

object evaluate(executor* Ex, expression* parsing_result, object scope, bool ast_only){
    if(parsing_result==NULL){
        return null_const;// there was an error while parsing
    }

    optimise_ast(parsing_result);
    if(g_print_ast){
        USING_STRING(stringify_expression(parsing_result, 0),
            printf("Abstract Syntax Tree:\n%s\n", str));
    }
    object execution_result;
    if(ast_only){
        ast_executor_state* state=malloc(sizeof(ast_executor_state));
        state->returning=false;
        object gcp;
        gcp.gcp=(gc_pointer*)state;
        gcp.gcp->destructor=(gc_pointer_destructor)free;
        gc_pointer_init(&gcp);
        TRY_CATCH(
            execution_result=execute_ast(Ex, state, parsing_result, scope, 1);
        ,
            printf("Error occured on line %i, column %i of source code:\n", state->line_number, state->column_number);
            printf(err_message);
            return null_const;
        );
        delete_expression(parsing_result);
    } else {
        bytecode_program prog=ast_to_bytecode(parsing_result, true);
        delete_expression(parsing_result);// at this point ast is useless and only wastes memory
        optimise_bytecode(&prog);

        if(g_print_bytecode){
            USING_STRING(stringify_bytecode(&prog),
                printf("Bytecode:\n%s\n", str));
        }
        
        bytecode_environment* environment=malloc(sizeof(bytecode_environment));
        environment->pointer=0;
        environment->program=malloc(sizeof(bytecode_program));
        memcpy(environment->program, &prog, sizeof(bytecode_program));
        environment->scope=scope;
        object gcp;
        gcp.gcp=(gc_pointer*)environment;
        gcp.gcp->destructor=(gc_pointer_destructor)bytecode_environment_free;
        gc_pointer_init(&gcp);

        bytecode_environment_init(environment);
        execution_result=execute_bytecode(Ex, environment);
    }
    return execution_result;
}

object evaluate_string(executor* Ex, const char* s, object scope){
    expression* parsing_result=parse_string(s);
    exec_state.file="string";
    return evaluate(Ex, parsing_result, scope, true);
}

object evaluate_file(executor* Ex, const char* file_name, object scope){
    expression* parsing_result=parse_file(file_name);
    exec_state.file=file_name;
    return evaluate(Ex, parsing_result, scope, g_ast_only);
}

void execute_file(executor* Ex, const char* file_name){
    object global_scope;
    table_init(&global_scope);
    reference(&global_scope);
    register_builtins(Ex, global_scope);
    object execution_result=evaluate_file(Ex, file_name, global_scope);
    USING_STRING(stringify(Ex, execution_result), 
        printf("Execution result:\n%s\n", str));
    USING_STRING(stringify(Ex, global_scope), 
       printf("Global scope:\n%s\n", str));
    dereference(Ex, &execution_result);
    dereference(Ex, &global_scope);
}

// this function should only be called from call_function, it's there to simplify the code structure
object call_function_processed(executor* Ex, function* f, object* arguments, int arguments_count){
    object function_scope;
    table_init(&function_scope);
    if(f->enclosing_scope.type!=t_null){
        inherit_scope(Ex, function_scope, f->enclosing_scope);
    }
    if(f->ftype==f_ast){
        for(int i=0; i<arguments_count; i++){
            STRING_OBJECT(argument_name, f->argument_names[i]);
            set(Ex, function_scope, argument_name, arguments[i]);
        }
        return execute_ast(Ex, (ast_executor_state*)f->environment, (expression*)f->source_pointer, function_scope, 1);
    } else if(f->ftype==f_bytecode){
        bytecode_environment* environment=(bytecode_environment*)f->environment;
        environment->scope=function_scope;

        move_to_function(Ex, environment, f, true);
        for(int i=0; i<arguments_count; i++){
            push(&environment->object_stack, arguments[i]);
        }
        object result=execute_bytecode(Ex, environment);
        return result;
    } else {
        THROW_ERROR(INCORRECT_OBJECT_POINTER, "Function type has incorrect value of %i", f->ftype);
        return null_const;
    }
}

// return error if arguments count is incorrect and proccess variadic functions, then call the function using call_function_processed
object call_function(executor* Ex, function* f, object* arguments, int arguments_count){
    if(f->ftype==f_native){
        return f->native_pointer(Ex, arguments, arguments_count);
    } else {
        int arguments_count_difference=arguments_count-f->arguments_count;
        if(f->variadic){
            if(arguments_count_difference<-1){
                RETURN_ERROR("CALL_ERROR", null_const, "Not enough arguments in variadic function call, expected at least %i, given %i.", f->arguments_count-1, arguments_count);
            }
            // make new arguments array
            int processed_arguments_count=f->arguments_count;
            object* processed_arguments=malloc(sizeof(object)*processed_arguments_count);
            // copy non variadic arguments
            for(int i=0; i<f->arguments_count-1; i++){
                processed_arguments[i]=arguments[i];
            }
            // pack variadic arguments into a table
            int variadic_arguments_count=arguments_count_difference+1;
            object variadic_table;
            table_init(&variadic_table);
            for(int i=variadic_arguments_count-1; i>=0; i--){
                set(Ex, variadic_table, to_number(i), arguments[f->arguments_count-1+i]);
            }
            // append the variadic array to the end of processed arguments array
            processed_arguments[f->arguments_count-1]=variadic_table;

            return call_function_processed(Ex, f, processed_arguments, processed_arguments_count);
        } else if(arguments_count_difference<0){
            RETURN_ERROR("CALL_ERROR", null_const, "Not enough arguments in function call, expected %i, given %i.", f->arguments_count, arguments_count);
        } else if(arguments_count_difference>0) {
            RETURN_ERROR("CALL_ERROR", null_const, "Too many arguments in function call, expected %i, given %i.", f->arguments_count, arguments_count);
        } else {
            return call_function_processed(Ex, f, arguments, arguments_count);
        }
    }
}

void deinit_function(function* f){
    // only ast functions own their source code for now
    // bytecode functions hold their source code in their shared environment
    if(f->ftype==f_ast){
        delete_expression(f->source_pointer);
    }
}