#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "ast/ast_to_source.h"

int main(){
    #if YYDEBUG == 1
       extern int yydebug;
       yydebug = 1;
    #endif
    
    printf("Read eval print loop of the duck parser. \n---\nType in duck syntax to see it being reinterpreted. \nWrite \"quit\" to exit the program.\n");
    REPL(
        TRY_CATCH({
            if(strcmp(input, "quit")==0){
                return 0;
            }
            Expression* parsing_result=parse_string(input);
            if(parsing_result!=NULL){
                USING_STRING(ast_to_source(parsing_result),
                    printf("%s\n", str))
            }
        },
            printf("Error occured:\n%s", err_message);
        )
    )
}