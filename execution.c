#include "execution.h"

object evaluate(expression* parsing_result, bool use_bytecode){
    if(parsing_result==NULL){
        return null_const;// there was an error while parsing
    }
    object global_scope;
    table_init(&global_scope);
    reference(&global_scope);
    register_builtins(global_scope);
    // chokes on bigger source trees
    /*USING_STRING(stringify_expression(parsing_result, 0),
        printf(str));*/
    optimise_ast(parsing_result);
    /*USING_STRING(stringify_expression(parsing_result, 0),
        printf("\nAFTER OPTIMISATIONS %s", str));*/
    printf("\nExecuting parsing result:\n");
    object execution_result;
    if(use_bytecode){
        bytecode_program prog=ast_to_bytecode(parsing_result, true);
        delete_expression(parsing_result);// at this point ast is useless and only wastes memory
        optimise_bytecode(&prog);
        USING_STRING(stringify_bytecode(&prog),
            printf("Bytecode:\n%s\n", str));
        
        bytecode_environment environment;
        environment.pointer=0;
        environment.program=&prog;
        environment.scope=global_scope;
        bytecode_enviroment_init(&environment);
        execution_result=execute_bytecode(&environment);
        
        USING_STRING(stringify(execution_result), 
            printf("Execution result:\n%s\n", str));
        USING_STRING(stringify(global_scope), 
            printf("Global scope:\n%s\n", str));
        
        bytecode_program_deinit(&prog);
    } else {
        ast_executor_state state;
        state.returning=false;
        TRY_CATCH(
            execution_result=execute_ast(&state, parsing_result, global_scope, 1);
            USING_STRING(stringify(execution_result), 
                printf("Execution result:\n%s\n", str));
            USING_STRING(stringify(global_scope), 
                printf("Global scope:\n%s\n", str));
        ,
            printf("Error occured on line %i, column %i of source code:\n", state.line_number, state.column_number);
            printf(err_message);
            exit(-1);
        );
        delete_expression(parsing_result);
    }
    
    //reference(&execution_result);// make sure that the execution_result isn't garbage collected along with global_scope
    dereference(&global_scope);
    return execution_result;
}

object evaluate_string(const char* s, bool use_bytecode){
    expression* parsing_result=parse_string(s);
    exec_state.file="string";
    return evaluate(parsing_result, use_bytecode);
}

object evaluate_file(const char* file_name, bool use_bytecode){
    expression* parsing_result=parse_file(file_name);
    exec_state.file=file_name;
    return evaluate(parsing_result, use_bytecode);
}

void execute_file(const char* file_name, bool use_bytecode){
    object execution_result=evaluate_file(file_name, use_bytecode);
    dereference(&execution_result);
}

// this function should only be called from call_function, it's there to simplify the code structure
object call_function_processed(function* f, object* arguments, int arguments_count){
    object function_scope;
    table_init(&function_scope);
    if(f->enclosing_scope.type!=t_null){
        inherit_scope(function_scope, f->enclosing_scope);
    }
    if(f->ftype==f_ast){
        for(int i=0; i<arguments_count; i++){
            STRING_OBJECT(argument_name, f->argument_names[i]);
            set(function_scope, argument_name, arguments[i]);
        }
        return execute_ast((ast_executor_state*)f->enviroment, (expression*)f->source_pointer, function_scope, 1);
    } else if(f->ftype==f_bytecode){
        bytecode_environment* environment=(bytecode_environment*)f->enviroment;
        environment->scope=function_scope;

        move_to_function(environment, f, true);
        for(int i=0; i<arguments_count; i++){
            push(&environment->object_stack, arguments[i]);
        }
        object result=execute_bytecode(environment);
        return result;
    } else {
        THROW_ERROR(INCORRECT_OBJECT_POINTER, "Function type has incorrect value of %i", f->ftype);
        return null_const;
    }
}

// return error if arguments count is incorrect and proccess variadic functions, then call the function using call_function_processed
object call_function(function* f, object* arguments, int arguments_count){
    if(f->ftype==f_native){
        return f->native_pointer(arguments, arguments_count);
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
                set(variadic_table, to_number(i), arguments[f->arguments_count-1+i]);
            }
            // append the variadic array to the end of processed arguments array
            processed_arguments[f->arguments_count-1]=variadic_table;

            return call_function_processed(f, processed_arguments, processed_arguments_count);
        } else if(arguments_count_difference<0){
            RETURN_ERROR("CALL_ERROR", null_const, "Not enough arguments in function call, expected %i, given %i.", f->arguments_count, arguments_count);
        } else if(arguments_count_difference>0) {
            RETURN_ERROR("CALL_ERROR", null_const, "Too many arguments in function call, expected %i, given %i.", f->arguments_count, arguments_count);
        } else {
            return call_function_processed(f, arguments, arguments_count);
        }
    }
}

void free_function(function* f){
    switch(f->ftype){
        case f_ast:
            delete_expression(f->source_pointer);
            break;
        case f_bytecode:
            bytecode_program_deinit(f->source_pointer);
            break;
        default:;
    }
}