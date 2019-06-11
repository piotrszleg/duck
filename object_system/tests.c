#include <stdio.h>
#include <assert.h>
#include "object.h"
#include "object_operations.h"
#include "../error/error.h"

struct Executor {
    GarbageCollector gc;
};

GarbageCollector* get_garbage_collector(Executor* E){
    return &E->gc;
}

Object call_function(Executor* E, Function* f, Object* arguments, int arguments_count){
    if(f->ftype==f_native){
        return f->native_pointer(E, arguments, arguments_count);
    } else {
        THROW_ERROR(NOT_IMPLEMENTED, "Calling functions other than native is not implemented.");
        return null_const;
    }
}

void deinit_function(Function* f){}

void get_execution_info(Executor* E, char* buffer, int buffer_count){
    strcat(buffer, "object_system testing unit");
}

Object call_coroutine(Executor* E, Coroutine* coroutine, Object* arguments, int arguments_count){
    return null_const;
}

void assert_stringification(Executor* E, Object o, char* expected){
    assert(strcmp(stringify(E, o), expected)==0);
}

void test_error_catching(){
    printf("TEST: %s\n", __FUNCTION__);

    TRY_CATCH(
        Object o;
        o.type=(ObjectType)100;
        is_falsy(o);// o has incorrect type value which should cause an error
    ,
        // TODO fix error type
        assert(err_type==INCORRECT_OBJECT_POINTER);
        printf("test successful\n");
        return;
    )
    assert(0);// there should be an error catched in the block above
}

void adding_numbers(Executor* E){// tests whether 1+2=3
    printf("TEST: %s\n", __FUNCTION__);
    
    Object num1;
    number_init(&num1);
    num1.value=1;
    Object num2;
    number_init(&num2);
    num2.value=2;
    assert_stringification(E, operator(E, num1, num2, "+"), "3");

    dereference(E, &num1);
    dereference(E, &num2);
    printf("test successful\n");
}

void adding_strings(Executor* E){// tests whether "Hello "+"Cruel World"="Hello Cruel World"
    printf("TEST: %s\n", __FUNCTION__);
    
    Object str1;
    string_init(&str1);
    str1.text="Hello ";
    Object str2;
    string_init(&str2);
    str2.text="Cruel World";
    assert_stringification(E, (operator(E, str1, str2, "+")), "\"Hello Cruel World\"");

    dereference(E, &str1);
    dereference(E, &str2);
    printf("test successful\n");
}

Object add_three(Executor* E, Object* arguments, int arguments_count){
    assert(arguments_count==1);
    Object three;
    number_init(&three);
    three.value=3;
    Object result= operator(E, arguments[0], three, "+");
    dereference(E, &three);
    return result;
}

void function_calling(Executor* E){// tests whether f(5)==8 where f(x)=x+3
    printf("TEST: %s\n", __FUNCTION__);

    Object f;
    function_init(E, &f);
    f.fp->native_pointer=add_three;
    f.fp->ftype=f_native;
    f.fp->arguments_count=1;

    Object five;
    number_init(&five);
    five.value=5;

    Object arguments[]={five};

    assert_stringification(E, call(E, f, arguments, 1), "8");

    dereference(E, &f);
    dereference(E, &five);
    printf("test successful\n");
}

void table_indexing(Executor* E){// t["name"]="John" => t["name"]=="John"
    printf("TEST: %s\n", __FUNCTION__);
    Object t;
    table_init(E, &t);

    STRING_OBJECT(name_key, "name");

    // test if setting works
    #define TEST_SET(key, value) \
        set(E, t, key, value); \
        assert(compare(get(E, t, key), value)==0);

    // getting variable at key that wasn't set before should return null
    #define TEST_EMPTY(key) assert(get(E, t, key).type==t_null);

    TEST_EMPTY(name_key)
    TEST_SET(name_key, to_string("John"))
    TEST_SET(name_key, to_number(2))
    TEST_EMPTY(to_number(0))
    TEST_SET(to_number(0), to_number(2))
    TEST_EMPTY(to_number(1))
    TEST_SET(to_number(1), to_number(2))

    dereference(E, &t);// deleting t will also delete n
    printf("test successful\n");
}

void adding_number_string(Executor* E){// tests whether "count: "+5="count: 5"
    printf("TEST: %s\n", __FUNCTION__);

    Object str;
    string_init(&str);
    str.text="count: ";

    Object num;
    number_init(&num);
    num.value=5;
    assert_stringification(E, operator(E, str, num, "+"), "\"count: 5\"");

    dereference(E, &num);
    dereference(E, &str);
    printf("test successful\n");
}

int main(){
    Executor E;
    object_system_init(&E);
    TRY_CATCH(
        // TODO test error objects
        object_system_init(&E);
        test_error_catching(&E);
        adding_numbers(&E);
        adding_strings(&E);
        function_calling(&E);
        table_indexing(&E);
        adding_number_string(&E);
    ,
        printf(err_message);
        exit(-1);
    )
    object_system_deinit(&E);
}
