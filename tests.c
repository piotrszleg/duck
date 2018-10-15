#include <stdio.h>
#include <assert.h>
#include "parser/parser.h"
#include "ast_executor.h"

void arithmetics(){
    printf("TEST: %s\n", __FUNCTION__);

    parse_string("1+1");
    
    table* global_scope=new_table();
    global_scope->ref_count++;
    register_globals(global_scope);
    object* execution_result=execute_ast(parsing_result, global_scope, 1);

    assert(execution_result->type==t_number);
    number* n=(number*)execution_result;
    assert(n->value==2);

    object_delete(global_scope);
    object_delete(execution_result);
    delete_expression(parsing_result);

    printf("test successful\n");
}

int main(){
    arithmetics();
}