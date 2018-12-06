#include <stdio.h>
#include <assert.h>
#include "object.h"
#include "object_operations.h"
#include "../error/error.h"

object call_function(function_* f, vector arguments){
    if(f->ftype==f_native){
        return f->pointer(arguments);
    } else {
        ERROR(NOT_IMPLEMENTED, "Calling functions other than native is not implemented.");
        return null_const;
    }
}

void assert_stringification(object o, char* expected){
    assert(strcmp(stringify(o), expected)==0);
}

void test_error_catching(){
    printf("TEST: %s\n", __FUNCTION__);
    object num;
    number_init(&num);
    object fun;
    function_init(&fun);

    TRY_CATCH(
        compare(num, fun);// comparing number to function should cause an error
        object_deinit(&num);
        object_deinit(&fun);
    ,
        // TODO fix error type
        assert(err_type==WRONG_ARGUMENT_TYPE);
        object_deinit(&num);
        object_deinit(&fun);
        printf("test successful\n");
        return;
    )
    assert(0);// there should be an error catched in the block above
}

void adding_numbers(){// tests whether 1+2=3
    printf("TEST: %s\n", __FUNCTION__);
    
    object num1;
    number_init(&num1);
    num1.value=1;
    object num2;
    number_init(&num2);
    num2.value=2;
    assert_stringification(operator(num1, num2, "+"), "3");

    object_deinit(&num1);
    object_deinit(&num2);
    printf("test successful\n");
}

void adding_strings(){// tests whether "Hello "+"Cruel World"="Hello Cruel World"
    printf("TEST: %s\n", __FUNCTION__);
    
    object str1;
    string_init(&str1);
    str1.text="Hello ";
    object str2;
    string_init(&str2);
    str2.text="Cruel World";
    assert_stringification((operator(str1, str2, "+")), "Hello Cruel World");

    object_deinit(&str1);
    object_deinit(&str2);
    printf("test successful\n");
}

object add_three(vector arguments){
    object three;
    number_init(&three);
    three.value=3;
    object result= operator(*(object*)vector_get(&arguments, 0), three, "+");
    object_deinit(&three);
    return result;
}

void function_call(){// tests whether f(5)==8 where f(x)=x+3
    printf("TEST: %s\n", __FUNCTION__);

    object f;
    function_init(&f);
    f.fp->pointer=add_three;
    f.fp->ftype=f_native;
    f.fp->arguments_count=1;

    object five;
    number_init(&five);
    five.value=5;

    vector arguments;
    vector_init(&arguments);
    vector_add(&arguments, &five);

    assert_stringification(f.fp->pointer(arguments), "8");

    object_deinit(&f);
    object_deinit(&five);
    printf("test successful\n");
}

void table_indexing(){// t["name"]="John" => t["name"]=="John"
    printf("TEST: %s\n", __FUNCTION__);
    object t;
    table_init(&t);

    assert(get(t, "name").type==t_null);// getting variable at key that wasn't set before should return null

    object s;
    string_init(&s);
    s.text="John";// set t["name"]="John"
    set(t, "name", s);
    assert_stringification((get(t, "name")), "John");// test if setting succeeded

    object n;
    number_init(&n);
    n.value=2;
    set(t, "name", n);// set t["name"]=2, check if it is possible to change variable type and value

    assert_stringification((get(t, "name")), "2");// test if setting succeeded

    object_deinit(&t);// deleting t will also delete n
    printf("test successful\n");
}

void adding_number_string(){// tests whether "count: "+5="count: 5"
    printf("TEST: %s\n", __FUNCTION__);

    object str;
    string_init(&str);
    str.text="count: ";

    object num;
    number_init(&num);
    num.value=5;
    assert_stringification(operator(str, num, "+"), "count: 5");

    object_deinit(&num);
    object_deinit(&str);
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
