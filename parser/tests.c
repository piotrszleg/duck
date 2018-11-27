#include <assert.h>
#include <string.h>
#include "parser.h"

extern expression* parsing_result;

block* parse_block(char* code, int expected_lines_count){
    parse_string(code);
    assert(parsing_result->type==_block);
    block* as_block=(block*)parsing_result;
    assert(vector_total(&as_block->lines)==expected_lines_count);
    return as_block;
}

void literals(){
    printf("TEST: %s\n", __FUNCTION__);

    block* as_block=parse_block("5, 0.5, \"text\"", 3);

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

    block* as_block=parse_block("2+2, 5.0**2, a*b, 2*a, a*2, a`atan`b, a==b, a!=b", 8);
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

    block* as_block=parse_block("a=2, a+=2, a**=2", 3);
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

    block* as_block=parse_block("-1, !a", 2);
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
    
    block* as_block=parse_block("[], [1, 2, \"John\"], [1, 2, 3, length=3]", 3);
    // first line should contain table_literal with 3 lines
    for (int i = 0; i < vector_total(&as_block->lines); i++){
        expression* e=(expression*)vector_get(&as_block->lines, i);
        assert(e->type==_table_literal);
    }

    delete_expression(parsing_result);
    printf("test successful\n");
}

void paths(){
    printf("TEST: %s\n", __FUNCTION__);

    block* as_block=parse_block("base.call, options[\"volume\"], options[user.name], base.graphics[\"effects\"]", 4);
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

    block* as_block=parse_block("->1, a->a+2, (a, b)->a+b, (a, b, c)->(a+b)/c", 4);
    // test if type of each expression is 'prefix' and compare operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_total(&as_block->lines); i++){
        expression* e=(expression*)vector_get(&as_block->lines, i);
        assert(e->type==_function_declaration);
    }

    delete_expression(parsing_result);
    printf("test successful\n");
}

void function_calls(){
    printf("TEST: %s\n", __FUNCTION__);

    int argument_counts[]={0, 1, 1, 2, 0, 1};
    block* as_block=parse_block("call(), call(0), call(name), add(1, 2), player.jump(), player.jump(10)", 6);

    // test if type of each expression is 'function_call' and number of arguments is correct
    for (int i = 0; i < vector_total(&as_block->lines); i++){
        expression* e=(expression*)vector_get(&as_block->lines, i);
        assert(e->type==_function_call);
        function_call* f=(function_call*)e;
        // test number of arguments of each call
        assert(vector_total(&f->arguments->lines)==argument_counts[i]);
    }

    delete_expression(parsing_result);
    printf("test successful\n");
}

void function_returns(){
    printf("TEST: %s\n", __FUNCTION__);

    block* as_block=parse_block("2! value! {a=2\n a!\n a}", 3);

    // test if type of each expression is 'function_return'
    for (int i = 0; i < vector_total(&as_block->lines)-1; i++){
        expression* e=(expression*)vector_get(&as_block->lines, i);
        assert(e->type==_function_return);
    }
    // test return parsing in the block in the last line
    expression* last_line_block=(expression*)vector_get(&as_block->lines, 2);
    assert(last_line_block->type==_block);
    // get the second line which should be a return statement
    expression* second_line=(expression*)vector_get(&((block*)last_line_block)->lines, 1);
    assert(second_line->type==_function_return);

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
    function_calls();
    function_returns();
    // TODO: conditionals
}