#include <assert.h>
#include <string.h>
#include "parser.h"

Block* parse_block(char* code, int expected_lines_count){
    Expression* parsing_result=parse_string(code);
    assert(parsing_result!=NULL);
    //printf("%s", stringify_expression(parsing_result, 0));
    
    assert(parsing_result->type==e_block);
    Block* as_block=(Block*)parsing_result;
    assert(vector_count(&as_block->lines)==expected_lines_count);
    return as_block;
}

void literals(){
    printf("TEST: %s\n", __FUNCTION__);

    Block* as_block=parse_block("5, 0.5, \"text text\", 'text_text", 4);

    // check if each line contains literal of correct type and value
    IntLiteral* five=(IntLiteral*)pointers_vector_get(&as_block->lines, 0);
    assert(five->type==e_int_literal);
    assert(five->value==5);
    
    FloatLiteral* half=(FloatLiteral*)pointers_vector_get(&as_block->lines, 1);
    assert(half->type==e_float_literal );
    assert(half->value==0.5);

    StringLiteral* text1=(StringLiteral*)pointers_vector_get(&as_block->lines, 2);
    assert( text1->type==e_string_literal );
    assert(strcmp(text1->value, "text text")==0);

    StringLiteral* text2=(StringLiteral*)pointers_vector_get(&as_block->lines, 3);
    assert( text2->type==e_string_literal );
    assert(strcmp(text2->value, "text_text")==0);

    delete_expression((Expression*)as_block);
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

    Block* as_block=parse_block("2+2, 5.0**2, a*b, 2*a, a*2, a`atan`b, a==b, a!=b", 8);
    // compare binary operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        Expression* e=(Expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_binary);
        assert(strcmp(((Binary*)e)->op, tested_operators[i])==0);
    }

    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

void assignment_operators(){
    printf("TEST: %s\n", __FUNCTION__);

    char *tested_operators[2]={
        "+",
        "**",
    };

    Block* as_block=parse_block("a=2, a+=2, a**=2", 3);
    // compare binary operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        Expression* e=(Expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_assignment);
        if(i>0){// ignore first assignment, because it doesn't contain any operators
            // test if right hand of the assignment is binary expression with correct operator
            Expression* right=((Assignment*)e)->right;
            assert(right->type==e_binary);
            assert(strcmp(((Binary*)right)->op, tested_operators[i-1])==0);
        }
    }

    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

void prefixes(){
    printf("TEST: %s\n", __FUNCTION__);

    char *tested_operators[2]={
        "-",
        "!",
    };

    Block* as_block=parse_block("-1, !a", 2);
    // test if type of each expression is 'prefix' and compare operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        Expression* e=(Expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_prefix);
        assert(strcmp(((Prefix*)e)->op, tested_operators[i])==0);
    }

    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

void table_literals(){
    printf("TEST: %s\n", __FUNCTION__);
    
    Block* as_block=parse_block("[], [1, 2, \"John\"], [1, 2, 3, length=3]", 3);
    // first line should contain table_literal with 3 lines
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        Expression* e=(Expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_table_literal);
    }

    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

void paths(){
    printf("TEST: %s\n", __FUNCTION__);

    Block* as_block=parse_block("base.call, options[\"volume\"], options[user.name], base.graphics[\"effects\"]", 4);
    ExpressionType line_types[]={
        e_member_access,
        e_indexer,
        e_indexer,
        e_indexer
    };
    // test if type of each expression is 'prefix' and compare operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        Expression* e=(Expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==line_types[i]);
    }

    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

void function_declarations(){
    printf("TEST: %s\n", __FUNCTION__);

    Block* as_block=parse_block("->1, a->a+2, (a, b)->a+b, (a, b, c)->(a+b)/c", 4);
    // test if type of each expression is 'prefix' and compare operators on each line to the ones in tested_operators array
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        Expression* e=(Expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_function_declaration);
    }

    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

void function_calls(){
    printf("TEST: %s\n", __FUNCTION__);

    int argument_counts[]={0, 1, 1, 2, 0, 1};
    Block* as_block=parse_block("call(), call(0), call(name), add(1, 2), player.jump(), player.jump(10)", 6);

    // test if type of each expression is 'function_call' and number of arguments is correct
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        Expression* e=(Expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_function_call);
        FunctionCall* f=(FunctionCall*)e;
        // test number of arguments of each call
        assert(vector_count(&f->arguments->lines)==argument_counts[i]);
    }

    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

void function_returns(){
    printf("TEST: %s\n", __FUNCTION__);

    Block* as_block=parse_block("2!\n value!\n {a=2\n a!\n a}", 3);

    // test if type of each expression is 'function_return'
    for (int i = 0; i < vector_count(&as_block->lines)-1; i++){
        Expression* e=(Expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_function_return);
    }
    // test return parsing in the block in the last line
    Expression* last_line_block=(Expression*)pointers_vector_get(&as_block->lines, 2);
    assert(last_line_block->type==e_block);
    // get the second line which should be a return statement
    Expression* second_line=(Expression*)pointers_vector_get(&((Block*)last_line_block)->lines, 1);
    assert(second_line->type==e_function_return);

    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

void conditionals(){
    printf("TEST: %s\n", __FUNCTION__);

    // how many elses are in each line
    int elses[]={0, 1, 2, 2, 3};

    Block* as_block=parse_block(
    "if(1) 1\n"
    "if(1) 1 else 2\n"
    "if(1) 1 elif(2) 2 else 3\n"
    "if(1){ 1 }elif(2){ 2 }else{ 3 }\n"
    "if(1){ 1 }elif(2){ 2 }elif(3){ 3 }else{ 4 }"
    , 5);

    for (int i = 0; i < vector_count(&as_block->lines); i++){
        Expression* e=(Expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_conditional);
        Conditional* c=(Conditional*)e;
        for(int e=0; e<elses[i]; e++){
            Expression* onfalse=c->onfalse;
            if(e<elses[i]-1) {
                // go one level deeper into conditional tree
                assert(onfalse->type==e_conditional);
                c=(Conditional*)onfalse;
            } else {
                assert(onfalse->type!=e_empty);
            }
        }
    }
    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

void blocks(){
    printf("TEST: %s\n", __FUNCTION__);

    int counts[]={2, 2, 2, 2, 1};

    Block* as_block=parse_block(
    "{1, 2}\n"
    "{\n1, 2}\n"
    "{\n1, 2\n}\n"
    "{\n1\n2\n}\n"
    "{\n1\n}\n"
    , 5);
    
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        Expression* e=(Expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_block);
        assert(vector_count(&((Block*)e)->lines)==counts[i]);
    }
    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

void messages(){
    printf("TEST: %s\n", __FUNCTION__);

    int counts[]={1, 2, 3, 3};

    Block* as_block=parse_block(
    "a::b()\n"
    "a::b(c)\n"
    "a::b(c, d)\n"
    "a.b::c(d, e)\n"
    , 4);
    for (int i = 0; i < vector_count(&as_block->lines); i++){
        Expression* e=(Expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_block);// TODO: make it more specific
        Block* message_block=(Block*)e;
        assert(vector_count(&message_block->lines)==2);
        Expression* second_line=pointers_vector_get(&message_block->lines, 1);
        assert(second_line->type==e_function_call);
        TableLiteral* arguments=((FunctionCall*)second_line)->arguments;
        assert(vector_count(&arguments->lines)==counts[i]);
    }
    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

void closures(){
    printf("TEST: %s\n", __FUNCTION__);


    #define ASSERT_CAST(expression, struct_name, type_tag) (assert((expression)->type==e_##type_tag), (struct_name*)(expression))
    #define GET_LINE(blc, index) (assert((blc)->type==e_block), (Expression*)pointers_vector_get(&((Block*)blc)->lines, index))
    #define FIRST_LINE(blck) GET_LINE(blck, 0)

    Block* as_block=parse_block(
    "{a=1, ->a}\n"
    "{->a=1, ->a}\n"
    "{a=1, ->(->a)}"
    , 3);
    
    // TODO: some kind of pattern matching traversal instead of these macros
    assert(ASSERT_CAST(FIRST_LINE(FIRST_LINE(as_block)), Assignment, assignment)->used_in_closure==true);
    assert(ASSERT_CAST(ASSERT_CAST(FIRST_LINE(GET_LINE(as_block, 1)), FunctionDeclaration, function_declaration)->body, Assignment, assignment)->used_in_closure==false);
    assert(ASSERT_CAST(FIRST_LINE(GET_LINE(as_block, 2)), Assignment, assignment)->used_in_closure==true);
    
    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

void return_if_errors(){
    printf("TEST: %s\n", __FUNCTION__);

    Block* as_block=parse_block(
    "1!?\n"
    "1!?, 1!?\n"
    , 3);

    for (int i = 0; i < vector_count(&as_block->lines); i++){
        Expression* e=(Expression*)pointers_vector_get(&as_block->lines, i);
        assert(e->type==e_return_if_error);
    }
    delete_expression((Expression*)as_block);
    printf("test successful\n");
}

Expression* type_exhausted_ast(){
    Block* root=new_block();
    #define EXPRESSION(struct_name, type_tag) \
        { struct_name* expression=new_##type_tag(); \
    
    #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) \
                                                         expression->field_name=new_##type_tag();
    #define EXPRESSION_FIELD(field_name)                 expression->field_name=new_expression();
    #define BOOL_FIELD(field_name)                       expression->field_name=false;
    #define STRING_FIELD(field_name)                     expression->field_name=strdup("test");
    #define FLOAT_FIELD(field_name)                      expression->field_name=0;
    #define INT_FIELD(field_name)                        expression->field_name=0;
    #define VECTOR_FIELD(field_name)
    #define END pointers_vector_push(&root->lines, expression); }

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

    return (Expression*)root;
}

Expression* type_exhausted_ast_uninitialized(){
    Block* root=new_block();
    #define EXPRESSION(struct_name, type_tag) \
        { struct_name* expression=new_##type_tag(); \
    
    #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name)
    #define EXPRESSION_FIELD(field_name)
    #define BOOL_FIELD(field_name)
    #define STRING_FIELD(field_name)
    #define FLOAT_FIELD(field_name)
    #define INT_FIELD(field_name)
    #define VECTOR_FIELD(field_name)
    #define END pointers_vector_push(&root->lines, expression); }

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

    return (Expression*)root;
}

void test_ast_functions(Expression* expression){
    char* stringified=stringify_expression(expression, 0);
    assert(stringified!=NULL);
    free(stringified);
    Expression* copy=copy_expression(expression);
    assert(expressions_equal(expression, copy));
    delete_expression(copy);
    delete_expression(expression);
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