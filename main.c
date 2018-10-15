#include "parser/parser.h"
#include "ast_executor.h"

int main(int argc, char *argv[]){
    if(argc>2){
        printf("Too many arguments.");
    } else if(argc<2){
        printf("Specify file to execute.");
    } else {
        parse_file(argv[1]);
    }
    TRY_CATCH(
        table* global_scope=new_table();
        global_scope->ref_count++;
        register_globals(global_scope);
        //printf(stringify_expression(parsing_result, 0));
        printf("\nExecuting parsing result:\n");
        table* execution_result=execute_ast(parsing_result, global_scope, 1);
        printf("Execution result:\n%s\n", stringify(execution_result));
        printf("Global scope:\n%s\n", stringify(global_scope));
        object_delete(global_scope);
        object_delete(execution_result);
        delete_expression(parsing_result);
    ,
        printf(err_message);
        int i=1/1;
        exit(-1);
    )
}