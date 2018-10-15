#include "repl.h"

void repl(){
    printf("Read eval print loop of the duck parser. \n---\nType in duck syntax to see it's AST representation. \nWrite \"quit\" to exit the program.\n");
    char input[128];

    table* global_scope=new_table();
    global_scope->ref_count++;
    register_globals(global_scope);

    vector asts;
    vector_init(&asts);

    while(1) {
        scanf("%s", input);
        if(strcmp(input, "quit")==0){
            break;
        }
        parse_string(input);

        TRY_CATCH(
            table* execution_result=execute_ast(parsing_result, global_scope, 1);
            printf("%s\n", stringify(execution_result));
            if(execution_result->ref_count<=1){
                object_delete(execution_result);
            }
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
    object_delete(global_scope);
}