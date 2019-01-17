#include <stdio.h>
#include <assert.h>
#include "object.h"
#include "object_operations.h"
#include "../error/error.h"

object call_function(function* f, object* arguments, int arguments_count){
    if(f->ftype==f_native){
        return f->native_pointer(arguments, arguments_count);
    } else {
        ERROR(NOT_IMPLEMENTED, "Calling functions other than native is not implemented.");
        return null_const;
    }
}

void free_function(function* f){}

void get_execution_info(char* buffer, int buffer_count){
    strcat(buffer, "object_system testing unit");
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
        STRING_OBJECT(s, "test")
        set(fun, s, num);// setting a field in function should cause an error
        dereference(&num);
        dereference(&fun);
    ,
        // TODO fix error type
        assert(err_type==WRONG_ARGUMENT_TYPE);
        dereference(&num);
        dereference(&fun);
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

    dereference(&num1);
    dereference(&num2);
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

    dereference(&str1);
    dereference(&str2);
    printf("test successful\n");
}

object add_three(object* arguments, int arguments_count){
    assert(arguments_count==1);
    object three;
    number_init(&three);
    three.value=3;
    object result= operator(arguments[0], three, "+");
    dereference(&three);
    return result;
}

void function_calling(){// tests whether f(5)==8 where f(x)=x+3
    printf("TEST: %s\n", __FUNCTION__);

    object f;
    function_init(&f);
    f.fp->native_pointer=add_three;
    f.fp->ftype=f_native;
    f.fp->arguments_count=1;

    object five;
    number_init(&five);
    five.value=5;

    object arguments[]={five};

    assert_stringification(call(f, arguments, 1), "8");

    dereference(&f);
    dereference(&five);
    printf("test successful\n");
}

void table_indexing(){// t["name"]="John" => t["name"]=="John"
    printf("TEST: %s\n", __FUNCTION__);
    object t;
    table_init(&t);

    STRING_OBJECT(name, "name");

    assert(get(t, name).type==t_null);// getting variable at key that wasn't set before should return null

    STRING_OBJECT(s, "John");
    set(t, name, s);
    assert_stringification((get(t, name)), "John");// test if setting succeeded

    object n;
    number_init(&n);
    n.value=2;
    set(t, name, n);// set t["name"]=2, check if it is possible to change variable type and value

    assert_stringification((get(t, name)), "2");// test if setting succeeded

    dereference(&t);// deleting t will also delete n
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

    dereference(&num);
    dereference(&str);
    printf("test successful\n");
}

int main(){
    TRY_CATCH(
        // TODO test error objects
        test_error_catching();
        adding_numbers();
        adding_strings();
        function_calling();
        table_indexing();
        adding_number_string();
    ,
        printf(err_message);
        exit(-1);
    )
}
