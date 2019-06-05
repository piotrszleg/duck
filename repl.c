#include "repl.h"

void repl(){
    printf("Read eval print loop of the duck parser. \n---\nType in duck expressions to get their evaluation results. \nWrite \"quit\" to exit the program.\n");
    char input[128];

    executor Ex;

    object global_scope;
    table_init(&global_scope);
    reference(&global_scope);
    register_builtins(&Ex, global_scope);

    while(1) {
        printf(">>");
        if(fgets_no_newline(input, sizeof(input), stdin)==NULL){
            continue;
        }
        if(strcmp(input, "quit")==0){
            break;
        }

        TRY_CATCH(
            
            object execution_result=evaluate_string(&Ex, input, global_scope);
            printf("%s\n", stringify(&Ex, execution_result));
            dereference(&Ex, &execution_result);
        ,
            printf("%s\n", err_message);
        )
    }
    dereference(&Ex, &global_scope);
}