#include <stdio.h>
#include <string.h>
#include "parser.h"

int main(){
    printf("Read eval print loop of the duck parser. \n---\nType in duck syntax to see it's AST representation. \nWrite \"quit\" to exit the program.\n");
    char input[128];
    while(1) {
        scanf("%s", input);
        if(strcmp(input, "quit")==0){
            break;
        }
        parse_string(input);
        printf("%s\n",stringify_expression(parsing_result, 0));
    }
}