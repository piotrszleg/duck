#include "parser/parser.h"
#include "ast_executor.h"

int main(){
    parse_file("input");
    TRY_CATCH(
        table* global_scope=new_table();
        register_globals(global_scope);
        //printf(stringify_expression(parsing_result, 0));
        printf("\nExecuting parsing result:\n");
        table* execution_result=execute_ast(parsing_result, global_scope);
        printf("Execution result:\n%s\n", stringify(execution_result));
        printf("Global scope:\n%s\n", stringify(global_scope));
        //collect_garbage(global_scope);
        delete_expression(parsing_result);
    ,
        printf(err_message);
        exit(-1);
    )
}