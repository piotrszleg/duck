#include "repl.h"

void repl(Executor* E){
    printf("Read-eval-print loop of the duck programming language. "
    "\n---\n"
    "Type in duck expressions to get their evaluation results.\n"
    "Ending the line with '\\' character will allow you to add next line.\n"
    "Call function \"exit\" with any argument to exit the program.\n");
    char input[256];

    Object global_scope;
    table_init(E, &global_scope);
    reference(&global_scope);
    if(E->options.include_builtins){
        inherit_scope(E, global_scope, builtins_table(E));
    }
    size_t input_offset=0;// used to write consecutive lines into input buffer
    while(1) {
        if(input_offset==0){
            printf(">>");
        } else {
            printf("  ");
        }
        if(fgets_no_newline(input+input_offset, sizeof(input)-input_offset, stdin)!=NULL){
            size_t input_length=strlen(input+input_offset);
            // if input line ends with '\' character the next line is appended to it after a newline
            if(input[input_offset+input_length-1]=='\\'){
                input[input_offset+input_length-1]='\n';
                input_offset+=input_length;
            } else {
                input_offset=0;
                TRY_CATCH(
                    Object execution_result=evaluate_string(E, input, global_scope);
                    printf("%s\n", stringify(E, execution_result));
                    dereference(E, &execution_result);
                ,
                    printf("%s\n", err_message);
                )
            }
        }
    }
    dereference(E, &global_scope);
}