#include <stdio.h>
#include <assert.h>
#include "object.h"
#include "object_operations.h"
#include "../error/error.h"

struct Executor {
    ObjectSystem beggining;
};

Object executor_get_patching_table(Executor* E){
    return null_const;
}

Object executor_on_unhandled_error(Executor* E, Object error) {
    USING_STRING(stringify(E, error),
        printf("Unhandled error:\n%s", str));
    return null_const;
}

Object call_function(Executor* E, Function* f, Object* arguments, int arguments_count){
    if(f->ftype==f_native){
        return f->native_pointer(E, f->enclosing_scope, arguments, arguments_count);
    } else {
        THROW_ERROR(NOT_IMPLEMENTED, "Calling functions other than native is not implemented.");
        return null_const;
    }
}

void coroutine_free(Coroutine* co) {}
void coroutine_foreach_children(Executor* E, Coroutine* co, ManagedPointerForeachChildrenCallback callback) {}

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
    
    Object num1=to_int(1);
    Object num2=to_int(2);
    assert_stringification(E, operator(E, num1, num2, "+"), "3");

    printf("test successful\n");
}

void adding_strings(Executor* E){// tests whether "Hello "+"Cruel World"="Hello Cruel World"
    printf("TEST: %s\n", __FUNCTION__);
    
    Object str1=to_string("Hello ");
    Object str2=to_string("Cruel World");
    assert_stringification(E, (operator(E, str1, str2, "+")), "\"Hello Cruel World\"");

    printf("test successful\n");
}

Object add_three(Executor* E, Object scope, Object* arguments, int arguments_count){
    assert(arguments_count==1);
    Object three=to_int(3);
    Object result= operator(E, arguments[0], three, "+");
    return result;
}

void function_calling(Executor* E){// tests whether f(5)==8 where f(x)=x+3
    printf("TEST: %s\n", __FUNCTION__);

    Object f;
    function_init(E, &f);
    f.fp->native_pointer=add_three;
    f.fp->ftype=f_native;
    f.fp->arguments_count=1;

    Object five=to_int(5);

    Object arguments[]={five};

    assert_stringification(E, call(E, f, arguments, 1), "8");

    dereference(E, &f);
    printf("test successful\n");
}

void table_indexing(Executor* E){// t["name"]="John" => t["name"]=="John"
    printf("TEST: %s\n", __FUNCTION__);
    Object t;
    table_init(E, &t);

    Object name_key=to_string("name");

    // test if setting works
    #define TEST_SET(key, value) \
        set(E, t, key, value); \
        assert(compare(E, get(E, t, key), value)==0);

    // getting variable at key that wasn't set before should return null
    #define TEST_EMPTY(key) assert(get(E, t, key).type==t_null);

    TEST_EMPTY(name_key)
    TEST_SET(name_key, to_string("John"))
    TEST_SET(name_key, to_int(2))
    TEST_EMPTY(to_int(0))
    TEST_SET(to_int(0), to_int(2))
    TEST_EMPTY(to_int(1))
    TEST_SET(to_int(1), to_int(2))

    dereference(E, &t);// deleting t will also delete n
    printf("test successful\n");
}

void adding_number_string(Executor* E){// tests whether "count: "+5="count: 5"
    printf("TEST: %s\n", __FUNCTION__);

    Object str=to_string("count: ");

    Object num=to_int(5);
    assert_stringification(E, operator(E, str, num, "+"), "\"count: 5\"");

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
        printf("%s", err_message);
        exit(-1);
    )
    object_system_deinit(&E);
}
