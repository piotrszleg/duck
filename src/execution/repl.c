#include "repl.h"

void repl(Executor* E){
    printf("Read-eval-print loop of the duck programming language. "
    "\n---\n"
    "Type in duck expressions to get their evaluation results.\n"
    "Ending the line with '\\' character will allow you to add next line.\n"
    "Call function \"exit\" with any argument to exit the program.\n");

    Object global_scope;
    table_init(E, &global_scope);
    reference(&global_scope);
    if(E->options.include_builtins){
        inherit_scope(E, global_scope.tp, builtins_table(E));
    }
    REPL(
        TRY_CATCH(
            Object execution_result=evaluate_string(E, input, global_scope);
            printf("%s\n", stringify(E, execution_result));
            dereference(E, &execution_result);
        ,
            printf("%s\n", err_message);
        )
    )
    dereference(E, &global_scope);
}