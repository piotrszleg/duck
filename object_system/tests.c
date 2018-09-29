#include "tests.h"

void test_error_catching(){
    printf("TEST: %s\n", __FUNCTION__);
    object* num1;

    TRY_CATCH(
        num1=object_new(t_number);
        ((number*)num1)->value=1;
        call(num1, num1);// try to call a number one, it should throw an error
    ,
        assert(err_type==WRONG_ARGUMENT_TYPE);
        object_delete(num1);
        printf("test successful\n");
        return;
    )
    assert(0);// there should be an error catched in the block above
}

void adding_numbers(){// tests whether 1+2=3
    printf("TEST: %s\n", __FUNCTION__);
    object* num1=object_new(t_number);
    ((number*)num1)->value=1;
    object* num2=object_new(t_number);
    ((number*)num2)->value=2;
    assert(strcmp(stringify(add(num1, num2)), "3")==0);
    object_delete(num1);
    object_delete(num2);
    printf("test successful\n");
}

void adding_strings(){// tests whether "Hello "+"Cruel World"="Hello Cruel World"
    printf("TEST: %s\n", __FUNCTION__);
    object* str1=object_new(t_string);
    ((string*)str1)->value="Hello ";
    object* str2=object_new(t_string);
    ((string*)str2)->value="Cruel World";
    assert(strcmp(stringify(add(str1, str2)), "Hello Cruel World")==0);
    object_delete(str1);
    object_delete(str2);
    printf("test successful\n");
}

object* add_three(object* o, table* arguments){
    object* three=object_new(t_number);
    ((number*)three)->value=3;
    return add(arguments, three);
}

void function_call(){// tests whether f(5)==8 where f(x)=x+3
    printf("TEST: %s\n", __FUNCTION__);
    object* f=object_new(t_function);
    ((function*)f)->pointer=add_three;

    object* five=object_new(t_number);
    ((number*)five)->value=5;

    assert(strcmp(stringify(call(f, five)), "8")==0);

    object_delete(f);
    object_delete(five);

    printf("test successful\n");
}

void table_indexing(){// t["name"]="John" => t["name"]=="John"
    printf("TEST: %s\n", __FUNCTION__);
    object* t=object_new(t_table);

    assert(get(t, "name")->type==t_null);// getting variable at key that wasn't set before should return null

    object* s=object_new(t_string);
    ((string*)s)->value="John";// set t["name"]="John"
    set(t, "name", s);
    assert(strcmp(stringify(get(t, "name")), "John")==0);// test if setting succeeded

    object* n=object_new(t_number);
    ((number*)n)->value=2;
    set(t, "name", n);// set t["name"]=2, check if it is possible to change variable type and value

    assert(strcmp(stringify(get(t, "name")), "2")==0);// test if setting succeeded

    object_delete(s);
    object_delete(n);
    object_delete(t);

    printf("test successful\n");
}

void adding_number_string(){// tests whether 1+2=3
    printf("TEST: %s\n", __FUNCTION__);

    object* str=object_new(t_string);
    ((string*)str)->value="count: ";

    object* num=object_new(t_number);
    ((number*)num)->value=5;
    assert(strcmp(stringify(add(str, num)), "count: 5")==0);

    object_delete(num);
    object_delete(str);

    printf("test successful\n");
}

void run_object_system_tests(){
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
