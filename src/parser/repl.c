#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "ast/ast_to_source.h"

int main(){
    #if YYDEBUG == 1
       extern int yydebug;
       yydebug = 1;
    #endif
    printf("Read eval print loop of the duck parser. \n---\nType in duck syntax to see it's AST representation. \nWrite \"quit\" to exit the program.\n");
    char input[128];
    while(1) {
        TRY_CATCH({
            printf(">>");
            fgets_no_newline(input, 128, stdin);
            if(strcmp(input, "quit")==0){
                break;
            }
            Expression* parsing_result=parse_string(input);
            if(parsing_result!=NULL){
                USING_STRING(ast_to_source(parsing_result),
                    printf("%s\n", str))
            }
        }, {
            printf("Error occured:\n%s", err_message);
        })
    }
}