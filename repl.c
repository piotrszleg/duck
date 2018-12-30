#include "repl.h"
#include "builtins.h"

void repl(int use_bytecode){
    printf("Read eval print loop of the duck parser. \n---\nType in duck syntax to see it's AST representation. \nWrite \"quit\" to exit the program.\n");
    char input[128];

    object global_scope;
    table_init(&global_scope);
    reference(&global_scope);
    register_builtins(global_scope);

    vector asts;
    vector_init(&asts);

    while(1) {
        scanf("%s", input);
        if(strcmp(input, "quit")==0){
            break;
        }
        parse_string(input);

        TRY_CATCH(
            ast_executor_state state;
            state.returning=false;
            object execution_result=execute_ast(&state, parsing_result, global_scope, 1);
            printf("%s\n", stringify(execution_result));
            object_deinit(&execution_result);
            vector_add(&asts, parsing_result);
        ,
            printf("%s\n", err_message);
        )
    }
    // since parts of ast are functions they can't be deleted right after executing each line
    // so they are deleted when program finishes
    for (int i = 0; i < vector_total(&asts); i++){
        delete_expression(vector_get(&asts, i));
    }
    object_deinit(&global_scope);
}