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

    TRY_CATCH(
        object o;
        o.type=(object_type)100;
        is_falsy(o);// o has incorrect type value which should cause an error
    ,
        // TODO fix error type
        assert(err_type==INCORRECT_OBJECT_POINTER);
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

    STRING_OBJECT(name_key, "name");

    // test if setting works
    #define TEST_SET(key, value) \
        set(t, key, value); \
        assert(compare(get(t, key), value)==0);

    // getting variable at key that wasn't set before should return null
    #define TEST_EMPTY(key) assert(get(t, key).type==t_null);

    TEST_EMPTY(name_key)
    TEST_SET(name_key, to_string("John"))
    TEST_SET(name_key, to_number(2))
    TEST_EMPTY(to_number(0))
    TEST_SET(to_number(0), to_number(2))
    TEST_EMPTY(to_number(1))
    TEST_SET(to_number(1), to_number(2))

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
        table_init(&patching_table);
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
