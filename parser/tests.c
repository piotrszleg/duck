#include <assert.h>
#include <string.h>
#include "parser.h"

block* parse_block(char* code, int expected_lines_count){
    expression* parsing_result=parse_string(code);
    assert(parsing_result!=NULL);
    //printf("%s", stringify_expression(parsing_result, 0));
    
    assert(parsing_result->type==e_block);
    block* as_block=(block*)parsing_result;
    assert(vector_count(&as_block->lines)==expected_lines_count);
    return as_block;
}

void literals(){
    printf("TEST: %s\n", __FUNCTION__);

    block* as_block=parse_block("5, 0.5, \"text text\", 'text_text", 4);

    // check if each line contains literal of correct type and value
    int_literal* five=(int_literal*)pointers_vector_get(&as_block->lines, 0);
    assert(five->type==e_int_literal);
    assert(five->value==5);
    
    float_literal* half=(float_literal*)pointers_vector_get(&as_block->lines, 1);
    assert(half->type==e_float_literal );
    assert(half->value==0.5);

    string_literal* text1=(string_literal*)pointers_vector_get(&as_block->lines, 2);
    assert( text1->type==e_string_literal );
    assert(strcmp(text1->value, "text text")==0);

    string_literal* text2=(string_literal*)pointers_vector_get(&as_block->lines, 3);
    assert( text2->type==e_string_literal );
    assert(strcmp(text2->value, "text_text")==0);

    delete_expression((expression*)as_block);
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
    // compare binary operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        expression* e=(expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_binary);
        assert(strcmp(((binary*)e)->op, tested_operators[i])==0);
    }

    delete_expression((expression*)as_block);
    printf("test successful\n");
}

void assignment_operators(){
    printf("TEST: %s\n", __FUNCTION__);

    char *tested_operators[2]={
        "+",
        "**",
    };

    block* as_block=parse_block("a=2, a+=2, a**=2", 3);
    // compare binary operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        expression* e=(expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_assignment);
        if(i>0){// ignore first assignment, because it doesn't contain any operators
            // test if right hand of the assignment is binary expression with correct operator
            expression* right=((assignment*)e)->right;
            assert(right->type==e_binary);
            assert(strcmp(((binary*)right)->op, tested_operators[i-1])==0);
        }
    }

    delete_expression((expression*)as_block);
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
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        expression* e=(expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_prefix);
        assert(strcmp(((prefix*)e)->op, tested_operators[i])==0);
    }

    delete_expression((expression*)as_block);
    printf("test successful\n");
}

void table_literals(){
    printf("TEST: %s\n", __FUNCTION__);
    
    block* as_block=parse_block("[], [1, 2, \"John\"], [1, 2, 3, length=3]", 3);
    // first line should contain table_literal with 3 lines
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        expression* e=(expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_table_literal);
    }

    delete_expression((expression*)as_block);
    printf("test successful\n");
}

void paths(){
    printf("TEST: %s\n", __FUNCTION__);

    block* as_block=parse_block("base.call, options[\"volume\"], options[user.name], base.graphics[\"effects\"]", 4);
    expression_type line_types[]={
        e_member_access,
        e_indexer,
        e_indexer,
        e_indexer
    };
    // test if type of each expression is 'prefix' and compare operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        expression* e=(expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==line_types[i]);
    }

    delete_expression((expression*)as_block);
    printf("test successful\n");
}

void function_declarations(){
    printf("TEST: %s\n", __FUNCTION__);

    block* as_block=parse_block("->1, a->a+2, (a, b)->a+b, (a, b, c)->(a+b)/c", 4);
    // test if type of each expression is 'prefix' and compare operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        expression* e=(expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_function_declaration);
    }

    delete_expression((expression*)as_block);
    printf("test successful\n");
}

void function_calls(){
    printf("TEST: %s\n", __FUNCTION__);

    int argument_counts[]={0, 1, 1, 2, 0, 1};
    block* as_block=parse_block("call(), call(0), call(name), add(1, 2), player.jump(), player.jump(10)", 6);

    // test if type of each expression is 'function_call' and number of arguments is correct
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        expression* e=(expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_function_call);
        function_call* f=(function_call*)e;
        // test number of arguments of each call
        assert(vector_count(&f->arguments->lines)==argument_counts[i]);
    }

    delete_expression((expression*)as_block);
    printf("test successful\n");
}

void function_returns(){
    printf("TEST: %s\n", __FUNCTION__);

    block* as_block=parse_block("2!\n value!\n {a=2\n a!\n a}", 3);

    // test if type of each expression is 'function_return'
    for (int i = 0; i < vector_count(&as_block->lines)-1; i++){
        expression* e=(expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_function_return);
    }
    // test return parsing in the block in the last line
    expression* last_line_block=(expression*)pointers_vector_get(&as_block->lines, 2);
    assert(last_line_block->type==e_block);
    // get the second line which should be a return statement
    expression* second_line=(expression*)pointers_vector_get(&((block*)last_line_block)->lines, 1);
    assert(second_line->type==e_function_return);

    delete_expression((expression*)as_block);
    printf("test successful\n");
}

void conditionals(){
    printf("TEST: %s\n", __FUNCTION__);

    // how many elses are in each line
    int elses[]={0, 1, 2, 2, 3};

    block* as_block=parse_block(
    "if(1) 1\n"
    "if(1) 1 else 2\n"
    "if(1) 1 elif(2) 2 else 3\n"
    "if(1){ 1 }elif(2){ 2 }else{ 3 }\n"
    "if(1){ 1 }elif(2){ 2 }elif(3){ 3 }else{ 4 }"
    , 5);

    for (int i = 0; i < vector_count(&as_block->lines); i++){
        expression* e=(expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_conditional);
        conditional* c=(conditional*)e;
        for(int e=0; e<elses[i]; e++){
            expression* onfalse=c->onfalse;
            if(e<elses[i]-1) {
                // go one level deeper into conditional tree
                assert(onfalse->type==e_conditional);
                c=(conditional*)onfalse;
            } else {
                assert(onfalse->type!=e_empty);
            }
        }
    }
    delete_expression((expression*)as_block);
    printf("test successful\n");
}

void blocks(){
    printf("TEST: %s\n", __FUNCTION__);

    int counts[]={2, 2, 2, 2, 1};

    block* as_block=parse_block(
    "{1, 2}\n"
    "{\n1, 2}\n"
    "{\n1, 2\n}\n"
    "{\n1\n2\n}\n"
    "{\n1\n}\n"
    , 5);
    
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        expression* e=(expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_block);
        assert(vector_count(&((block*)e)->lines)==counts[i]);
    }
    delete_expression((expression*)as_block);
    printf("test successful\n");
}

void messages(){
    printf("TEST: %s\n", __FUNCTION__);

    int counts[]={1, 2, 3, 3};

    block* as_block=parse_block(
    "a::b()\n"
    "a::b(c)\n"
    "a::b(c, d)\n"
    "a.b::c(d, e)\n"
    , 4);
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        expression* e=(expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_block);// TODO: make it more specific
        block* message_block=(block*)e;
        assert(vector_count(&message_block->lines)==2);
        expression* second_line=pointers_vector_get(&message_block->lines, 1);
        assert(second_line->type==e_function_call);
        table_literal* arguments=((function_call*)second_line)->arguments;
        assert(vector_count(&arguments->lines)==counts[i]);
    }
    delete_expression((expression*)as_block);
    printf("test successful\n");
}

void closures(){
    printf("TEST: %s\n", __FUNCTION__);


    #define ASSERT_CAST(exp, expected_type) (assert((exp)->type==e_##expected_type), (expected_type*)(exp))
    #define GET_LINE(blc, index) (assert((blc)->type==e_block), (expression*)pointers_vector_get(&((block*)blc)->lines, index))
    #define FIRST_LINE(blck) GET_LINE(blck, 0)

    block* as_block=parse_block(
    "{a=1, ->a}\n"
    "{->a=1, ->a}\n"
    "{a=1, ->(->a)}"
    , 3);
    
    // TODO: some kind of pattern matching traversal instead of these macros
    assert(ASSERT_CAST(FIRST_LINE(FIRST_LINE(as_block)), assignment)->used_in_closure=true);
    assert(ASSERT_CAST(ASSERT_CAST(FIRST_LINE(GET_LINE(as_block, 1)), function_declaration)->body, assignment)->used_in_closure==false);
    assert(ASSERT_CAST(FIRST_LINE(GET_LINE(as_block, 2)), assignment)->used_in_closure=true);
    
    delete_expression((expression*)as_block);
    printf("test successful\n");
}

void return_if_errors(){
    printf("TEST: %s\n", __FUNCTION__);

    block* as_block=parse_block(
    "1!?\n"
    "1!?, 1!?\n"
    , 3);

    for (int i = 0; i < vector_count(&as_block->lines); i++){
        expression* e=(expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_return_if_error);
    }
    delete_expression((expression*)as_block);
    printf("test successful\n");
}

expression* type_exhausted_ast(){
    block* root=new_block();
    #define EXPRESSION(type) \
        { type* exp=new_##type(); \
    
    #define SPECIFIED_EXPRESSION_FIELD(type, field_name) exp->field_name=new_##type();
    #define EXPRESSION_FIELD(field_name)                 exp->field_name=new_expression();
    #define BOOL_FIELD(field_name)                       exp->field_name=false;
    #define STRING_FIELD(field_name)                     exp->field_name=strdup("test");
    #define FLOAT_FIELD(field_name)                      exp->field_name=0;
    #define INT_FIELD(field_name)                        exp->field_name=0;
    #define VECTOR_FIELD(field_name)
    #define END pointers_vector_push(&root->lines, exp); }

    AST_EXPRESSIONS

    #undef EXPRESSION
    #undef SPECIFIED_EXPRESSION_FIELD
    #undef EXPRESSION_FIELD               
    #undef BOOL_FIELD                   
    #undef STRING_FIELD
    #undef VECTOR_FIELD
    #undef FLOAT_FIELD
#undef INT_FIELD
    #undef END

    return (expression*)root;
}

expression* type_exhausted_ast_uninitialized(){
    block* root=new_block();
    #define EXPRESSION(type) \
        { type* exp=new_##type(); \
    
    #define SPECIFIED_EXPRESSION_FIELD(type, field_name)
    #define EXPRESSION_FIELD(field_name)
    #define BOOL_FIELD(field_name)
    #define STRING_FIELD(field_name)
    #define FLOAT_FIELD(field_name)
    #define INT_FIELD(field_name)
    #define VECTOR_FIELD(field_name)
    #define END pointers_vector_push(&root->lines, exp); }

    AST_EXPRESSIONS

    #undef EXPRESSION
    #undef SPECIFIED_EXPRESSION_FIELD
    #undef EXPRESSION_FIELD               
    #undef BOOL_FIELD                   
    #undef STRING_FIELD
    #undef VECTOR_FIELD
    #undef FLOAT_FIELD
    #undef INT_FIELD
    #undef END

    return (expression*)root;
}

void test_ast_functions(expression* exp){
    char* stringified=stringify_expression(exp, 0);
    assert(stringified!=NULL);
    free(stringified);
    expression* copy=copy_expression(exp);
    assert(expressions_equal(exp, copy));
    delete_expression(copy);
    delete_expression(exp);
}

void ast_tests(){
    printf("TEST: %s\n", __FUNCTION__);

    test_ast_functions(type_exhausted_ast());
    assert(ast_allocations_zero());
    test_ast_functions(type_exhausted_ast_uninitialized());
    assert(ast_allocations_zero());

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
    conditionals();
    blocks();
    messages();
    ast_tests();
    closures();
    return_if_errors();
}