#include "parser/parser.h"
#include "ast_executor.h"

int main(){
    parse_file("input");
    TRY_CATCH(
        //printf(stringify_expression(result, 0));
        table* global_scope=new_table();
        register_globals(global_scope);
        printf("\nExecuting parsing result:\n");
        table* execution_result=execute_ast(parsing_result, global_scope);
        printf(stringify(execution_result));
        collect_garbage(execution_result);
        delete_expression(parsing_result);
    ,
        printf(err_message);
        exit(-1);
    )
}