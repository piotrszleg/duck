#include <assert.h>
#include <string.h>
#include "parser.h"

extern expression* parsing_result;

void literals(){
    printf("TEST: %s\n", __FUNCTION__);

    parse_string("5, 0.5, \"text\"");
    assert(parsing_result->type==_block);
    block* as_block=(block*)parsing_result;
    assert(vector_total(&as_block->lines)==3);

    // check if each line contains literal of correct type and value

    assert( ((expression*)vector_get(&as_block->lines, 0))->type==_literal );
    literal* five=(literal*)vector_get(&as_block->lines, 0);
    assert(five->ival==5);

    assert( ((expression*)vector_get(&as_block->lines, 1))->type==_literal );
    literal* half=(literal*)vector_get(&as_block->lines, 1);
    assert(half->fval==0.5);

    assert( ((expression*)vector_get(&as_block->lines, 2))->type==_literal );
    literal* text=(literal*)vector_get(&as_block->lines, 2);
    assert(strcmp(text->sval, "text")==0);

    delete_expression(parsing_result);
    printf("test successful\n");
}

void operators(){
    printf("TEST: %s\n", __FUNCTION__);

    char *tested_operators[8]={
        "+",
        "**",
        "*",
        "*",
        "*",
        "atan",
        "==",
        "!="
    };

    parse_string("2+2, 5.0**2, a*b, 2*a, a*2, a`atan`b, a==b, a!=b");
    assert(parsing_result->type==_block);
    block* as_block=(block*)parsing_result;
    assert(vector_total(&as_block->lines)==8);

    // compare unary operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_total(&as_block->lines); i++){
        expression* e=(expression*)vector_get(&as_block->lines, i);
        assert(e->type==_unary);
        assert(strcmp(((unary*)e)->op, tested_operators[i])==0);
    }

    delete_expression(parsing_result);
    printf("test successful\n");
}

void assignment_operators(){
    printf("TEST: %s\n", __FUNCTION__);

    char *tested_operators[2]={
        "+",
        "**",
    };

    parse_string("a=2, a+=2, a**=2");
    assert(parsing_result->type==_block);
    block* as_block=(block*)parsing_result;
    assert(vector_total(&as_block->lines)==3);

    // compare unary operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_total(&as_block->lines); i++){
        expression* e=(expression*)vector_get(&as_block->lines, i);
        assert(e->type==_assignment);
        if(i>0){// ignore first assignment, because it doesn't contain any operators
            // test if right hand of the assignment is unary expression with correct operator
            expression* right=((assignment*)e)->right;
            assert(right->type==_unary);
            assert(strcmp(((unary*)right)->op, tested_operators[i-1])==0);
        }
    }

    delete_expression(parsing_result);
    printf("test successful\n");
}

void prefixes(){
    printf("TEST: %s\n", __FUNCTION__);

    char *tested_operators[2]={
        "-",
        "!",
    };

    parse_string("-1, !a");
    assert(parsing_result->type==_block);
    block* as_block=(block*)parsing_result;
    assert(vector_total(&as_block->lines)==2);

    // test if type of each expression is 'prefix' and compare operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_total(&as_block->lines); i++){
        expression* e=(expression*)vector_get(&as_block->lines, i);
        assert(e->type==_prefix);
        assert(strcmp(((prefix*)e)->op, tested_operators[i])==0);
    }

    delete_expression(parsing_result);
    printf("test successful\n");
}

void table_literals(){
    printf("TEST: %s\n", __FUNCTION__);

    parse_string("[1, 2, \"John\"]");

    // result of parsing always has root block expression on top
    assert(parsing_result->type==_block);
    block* as_block=(block*)parsing_result;
    assert(vector_total(&as_block->lines)==1);

    // first line should contain table_literal with 3 lines
    expression* first_line=vector_get(&as_block->lines, 0);
    assert(first_line->type==_table_literal);
    assert(vector_total(&((table_literal*)first_line)->lines)==3);

    printf("test successful\n");
}

void paths(){
    printf("TEST: %s\n", __FUNCTION__);

    parse_string("base.call, options[\"volume\"], options[user.name], base.graphics[\"effects\"]");
    assert(parsing_result->type==_block);
    block* as_block=(block*)parsing_result;
    assert(vector_total(&as_block->lines)==4);

    // test if type of each expression is 'prefix' and compare operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_total(&as_block->lines); i++){
        expression* e=(expression*)vector_get(&as_block->lines, i);
        assert(e->type==_path);
    }

    delete_expression(parsing_result);
    printf("test successful\n");
}

void function_declarations(){
    printf("TEST: %s\n", __FUNCTION__);

    parse_string("->1, a->a+2, (a, b)->a+b, (a, b, c)->(a+b)/c");
    assert(parsing_result->type==_block);
    block* as_block=(block*)parsing_result;
    assert(vector_total(&as_block->lines)==4);

    // test if type of each expression is 'prefix' and compare operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_total(&as_block->lines); i++){
        expression* e=(expression*)vector_get(&as_block->lines, i);
        assert(e->type==_function_declaration);
    }

    delete_expression(parsing_result);
    printf("test successful\n");
}


int main(){
    literals();
    operators();
    prefixes();
    assignment_operators();
    table_literals();
    paths();
    function_declarations();
    // TODO: function calling, conditionals
    // function declarations
}