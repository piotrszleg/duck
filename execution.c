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
        bytecode_program prog=ast_to_bytecode(parsing_result, 1);
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
    
    reference(&execution_result);// make sure that the execution_result isn't garbage collected along with global_scope
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

object call_function(function* f, object* arguments, int arguments_count){
    if(f->ftype==f_native){
        return f->native_pointer(arguments, arguments_count);
    } else {
        object function_scope;
        table_init(&function_scope);
        if(f->enclosing_scope.type!=t_null){
            inherit_scope(function_scope, f->enclosing_scope);
        }
        if(f->ftype==f_ast){
            if(f->arguments_count<arguments_count){
                ERROR(NOT_ENOUGH_ARGUMENTS, "Too many arguments in ast function call.");
                return null_const;
            }
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
            ERROR(INCORRECT_OBJECT_POINTER, "Function type has incorrect value of %i", f->ftype);
            return null_const;
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