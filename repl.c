#include "repl.h"
#include "builtins.h"

void repl(int use_bytecode){
    printf("Read eval print loop of the duck parser. \n---\nType in duck syntax to see it's AST representation. \nWrite \"quit\" to exit the program.\n");
    char input[128];

    object global_scope;
    table_init(&global_scope);
    reference(&global_scope);
    register_builtins(global_scope);

    while(1) {
        scanf("%s", input);
        if(strcmp(input, "quit")==0){
            break;
        }
        expression* parsing_result=parse_string(input);
        if(parsing_result==NULL){
            continue;// ignore rest of the loop if there was a syntax error
        }

        TRY_CATCH(
            ast_executor_state state;
            state.returning=false;
            object execution_result=execute_ast(&state, parsing_result, global_scope, 1);
            printf("%s\n", stringify(execution_result));
            dereference(&execution_result);
            delete_expression(parsing_result);
        ,
            printf("%s\n", err_message);
        )
    }
    // since parts of ast are functions they can't be deleted right after executing each line
    // so they are deleted when program finishes
    dereference(&global_scope);
}