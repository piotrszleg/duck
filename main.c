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
        printf(stringify(execution_result));
        //collect_garbage(global_scope);
        delete_expression(parsing_result);
    ,
        printf(err_message);
        exit(-1);
    )
}