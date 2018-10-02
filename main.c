#include "parser/parser.h"
#include "ast_executor.h"

void execute_parsing_result(expression* result){
    TRY_CATCH(
        //printf(stringify_expression(result, 0));
        table* global_scope=new_table();
        register_globals(global_scope);
        printf("\nExecuting parsing result:\n");
        table* execution_result=execute_ast(result, global_scope);
        printf(stringify(execution_result));
        collect_garbage(execution_result);
        delete_expression(result);
    ,
        printf(err_message);
        exit(-1);
    )
}

int main(){
    //run_object_system_tests();
    //run_parser_tests();
    parse_file("input", execute_parsing_result);
}