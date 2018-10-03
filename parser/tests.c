#include <assert.h>
#include "parser.h"

extern expression* parsing_result;

void literals_parsing(){// tests literals parsing
    printf("TEST: %s\n", __FUNCTION__);

    parse_string("5");
    assert(((block*)parsing_result));// todo examine the inner structure

    delete_expression(parsing_result);
    printf("test successful\n");
}

int main(){
    literals_parsing();
}