#include "parser.h"

void print_parsing_result(expression* result){
    printf(stringify_expression(result, 0));
    delete_expression(result);
}


int main(){
    parse_file("input", print_parsing_result);
}