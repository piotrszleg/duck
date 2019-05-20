#include <stdio.h>
#include <string.h>
#include "parser.h"

// source: https://stackoverflow.com/questions/1694036/why-is-the-gets-function-so-dangerous-that-it-should-not-be-used
char* fgets_no_newline(char *buffer, size_t buflen, FILE* fp) {
    if (fgets(buffer, buflen, fp) != 0)
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        return buffer;
    }
    return 0;
}

int main(){
    #if YYDEBUG == 1
       extern int yydebug;
       yydebug = 1;
    #endif
    printf("Read eval print loop of the duck parser. \n---\nType in duck syntax to see it's AST representation. \nWrite \"quit\" to exit the program.\n");
    char input[128];
    while(1) {
        fgets_no_newline(input, 128, stdin);
        if(strcmp(input, "quit")==0){
            break;
        }
        expression* parsing_result=parse_string(input);
        if(parsing_result!=NULL){
            printf("%s\n",stringify_expression(parsing_result, 0));
        }
    }
}