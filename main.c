#include <string.h>
#include "parser/parser.h"
#include "execute_ast.h"
#include "execute_ast.h"
#include "repl.h"
#include "bytecode.h"
#include "ast_to_bytecode.h"
#include "execute_bytecode.h"
#include "optimisations/ast_optimisations.h"
#include "optimisations/bytecode_optimisations.h"
#include "builtins.h"
#include "macros.h"
#include "object_system/object_operations.h"

void execute_file(const char* file_name, int use_bytecode){
    parse_file(file_name);
    object global_scope;
    table_init(&global_scope);
    reference(&global_scope);
    register_builtins(global_scope);
    // chokes on bigger source trees
    USING_STRING(stringify_expression(parsing_result, 0),
        printf(str));
    optimise_ast(parsing_result);
    USING_STRING(stringify_expression(parsing_result, 0),
        printf("\nAFTER OPTIMISATIONS %s", str));
    printf("\nExecuting parsing result:\n");
    object execution_result;
    if(use_bytecode){
        bytecode_program prog=ast_to_bytecode(parsing_result, 1);
        delete_expression(parsing_result);// at this point ast is useless and only wastes memory
        optimise_bytecode(&prog);
        /*USING_STRING(stringify_bytecode(&prog),
            printf("Bytecode:\n%s\n", str));*/
        
        bytecode_environment environment;
        environment.pointer=0;
        environment.code=prog.code;
        environment.constants=prog.constants;
        bytecode_enviroment_init(&environment);
        execution_result=execute_bytecode(&environment, global_scope);

        USING_STRING(stringify(execution_result), 
            printf("Execution result:\n%s\n", str));
        USING_STRING(stringify(global_scope), 
            printf("Global scope:\n%s\n", str));
        
        free(prog.code);
        free((char*)prog.constants);
    } else {
        ast_executor_state state;
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
    object_deinit(&global_scope);
    object_deinit(&execution_result);
}

object call_function(function_* f, object* arguments, int arguments_count){
    if(f->ftype==f_native){
        return f->pointer(arguments, arguments_count);
    } else {
        object function_scope;
        table_init(&function_scope);
        if(f->enclosing_scope!=NULL){
            inherit_scope(function_scope, *f->enclosing_scope);
        }
        if(f->ftype==f_ast){
            if(vector_total(&f->argument_names)<arguments_count){
                ERROR(NOT_ENOUGH_ARGUMENTS, "Too many arguments in ast function call.");
                return null_const;
            }
            for(int i=0; i<arguments_count; i++){
                set(function_scope, vector_get(&f->argument_names, i), arguments[i]);
            }
            return execute_ast((ast_executor_state*)f->enviroment, (expression*)f->ast_pointer, function_scope, 1);
        } else if(f->ftype==f_bytecode){
            bytecode_environment* environment=(bytecode_environment*)f->enviroment;
            int previous_pointer=environment->pointer;
            environment->pointer=environment->labels[f->label];// move to function's label
            for(int i=0; i<arguments_count; i++){
                push(&environment->object_stack, arguments[i]);
            }
            object result=execute_bytecode(environment, function_scope);
            environment->pointer=previous_pointer;
            return result;
        } else {
            ERROR(INCORRECT_OBJECT_POINTER, "Function type has incorrect value of %i", f->ftype);
            return null_const;
        }
    }
}

int main(int argc, char *argv[]){
    if(argc>3){
        printf("Too many arguments.");
    } else {
        TRY_CATCH(
            int use_bytecode=strcmp(argv[argc-1], "-ast")!=0;
            if(!use_bytecode){
                if(argc==3){
                    execute_file(argv[1], use_bytecode);
                } else {
                    repl(use_bytecode);
                }
            } else {
                if(argc>2){
                    printf("Flags should be placed after the filename.");
                } else if(argc==2){
                    execute_file(argv[1], use_bytecode);
                } else {
                    repl(use_bytecode);
                }
            }
        ,
            printf("Error occured:\n");
            printf(err_message);
            exit(-1);
        );
    }
}