#include <stdio.h>
#include <assert.h>
#include "object.h"
#include "../error/error.h"

object* call_function(function* f, vector arguments){
    if(f->ftype==f_native){
        return f->pointer(arguments);
    } else {
        ERROR(NOT_IMPLEMENTED, "Calling functions other than native is not implemented.");
        return (object*)new_null();
    }
}

void assert_stringification(object* o, char* expected){
    assert(strcmp(stringify(o), expected)==0);
}

void test_error_catching(){
    printf("TEST: %s\n", __FUNCTION__);
    number* num1;

    TRY_CATCH(
        num1=new_number();
        num1->value=1;
        object* casted=cast((object*)num1, t_function);// try to call a number one, it should throw an error
        object_delete((object*)casted);
    ,
        // TODO fix error type
        // assert(err_type==WRONG_ARGUMENT_TYPE);
        object_delete((object*)num1);
        printf("test successful\n");
        return;
    )
    assert(0);// there should be an error catched in the block above
}

void adding_numbers(){// tests whether 1+2=3
    printf("TEST: %s\n", __FUNCTION__);
    
    number* num1=new_number();
    num1->value=1;
    number* num2=new_number();
    num2->value=2;
    assert_stringification(operator((object*)num1, (object*)num2, "+"), "3");

    object_delete((object*)num1);
    object_delete((object*)num2);
    printf("test successful\n");
}

void adding_strings(){// tests whether "Hello "+"Cruel World"="Hello Cruel World"
    printf("TEST: %s\n", __FUNCTION__);
    
    string* str1=new_string();
    str1->value="Hello ";
    string* str2=new_string();
    str2->value="Cruel World";
    assert_stringification((operator((object*)str1, (object*)str2, "+")), "Hello Cruel World");

    object_delete((object*)str1);
    object_delete((object*)str2);
    printf("test successful\n");
}

object* add_three(vector arguments){
    number* three=new_number();
    three->value=3;
    object* result= operator(vector_get(&arguments, 0), (object*)three, "+");
    object_delete((object*)three);
    return result;
}

void function_call(){// tests whether f(5)==8 where f(x)=x+3
    printf("TEST: %s\n", __FUNCTION__);

    function* f=new_function();
    f->pointer=add_three;
    f->type=f_native;
    f->arguments_count=1;

    number* five=new_number();
    five->value=5;

    vector arguments;
    vector_init(&arguments);
    vector_add(&arguments, (object*)five);

    assert_stringification(f->pointer(arguments), "8");

    object_delete((object*)f);
    object_delete((object*)five);
    printf("test successful\n");
}

void table_indexing(){// t["name"]="John" => t["name"]=="John"
    printf("TEST: %s\n", __FUNCTION__);
    table* t=new_table();

    assert(get((object*)t, "name")->type==t_null);// getting variable at key that wasn't set before should return null

    string* s=new_string();
    s->value="John";// set t["name"]="John"
    set((object*)t, "name", (object*)s);
    assert_stringification((get((object*)t, "name")), "John");// test if setting succeeded

    number* n=new_number();
    n->value=2;
    set((object*)t, "name", (object*)n);// set t["name"]=2, check if it is possible to change variable type and value

    assert_stringification((get((object*)t, "name")), "2");// test if setting succeeded

    object_delete((object*)s);
    object_delete((object*)t);// deleting t will also delete n
    printf("test successful\n");
}

void adding_number_string(){// tests whether "count: "+5="count: 5"
    printf("TEST: %s\n", __FUNCTION__);

    string* str=new_string();
    str->value="count: ";

    number* num=new_number();
    num->value=5;
    assert_stringification(operator((object*)str, (object*)num, "+"), "count: 5");

    object_delete((object*)num);
    object_delete((object*)str);
    printf("test successful\n");
}

int main(){
    TRY_CATCH(
        test_error_catching();
        adding_numbers();
        adding_strings();
        function_call();
        table_indexing();
        adding_number_string();
    ,
        printf(err_message);
        exit(-1);
    )
}
