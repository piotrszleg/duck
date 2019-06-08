#include "repl.h"

void repl(){
    printf("Read eval print loop of the duck parser. \n---\nType in duck expressions to get their evaluation results. \nWrite \"quit\" to exit the program.\n");
    char input[128];

    Executor E;
    E.options=default_options;
    E.options.ast_only=true;
    E.gc=malloc(sizeof(Executor));
    object_system_init(&E);
    Object global_scope;
    table_init(&E, &global_scope);
    reference(&global_scope);
    register_builtins(&E, global_scope);

    while(1) {
        printf(">>");
        if(fgets_no_newline(input, sizeof(input), stdin)==NULL){
            continue;
        }
        if(strcmp(input, "quit")==0){
            break;
        }

        TRY_CATCH(
            Object execution_result=evaluate_string(&E, input, global_scope);
            printf("%s\n", stringify(&E, execution_result));
            dereference(&E, &execution_result);
        ,
            printf("%s\n", err_message);
        )
    }
    dereference(&E, &global_scope);
    object_system_deinit(&E);
    free(E.gc);
}