#include <stdio.h>
#include <string.h>
#include "parser.h"

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
                printf("%s\n",stringify_expression(parsing_result, 0));
            }
        }, {
            printf("Error occured:\n");
            printf(err_message);
        })
    }
}