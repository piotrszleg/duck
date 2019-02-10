#include "repl.h"
#include "runtime/builtins.h"

void repl(int use_bytecode){
    printf("Read eval print loop of the duck parser. \n---\nType in duck expressions to get their evaluation results. \nWrite \"quit\" to exit the program.\n");
    char input[128];

    object global_scope;
    table_init(&global_scope);
    reference(&global_scope);
    register_builtins(global_scope);

    while(1) {
        if(fgets_no_newline(input, sizeof(input), stdin)==NULL){
            continue;
        }
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
    dereference(&global_scope);
}