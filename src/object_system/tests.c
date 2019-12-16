#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "object.h"
#include "object_operations.h"
#include "../error/error.h"
#include "../utility.h"

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
        CRITICAL_ERROR(NOT_IMPLEMENTED, "Calling functions other than native is not implemented.");
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

#define assert_equal(E, a, b) \
    assert(compare(E, a, b)==0)

void operators(Executor* E){
    #define OPERATOR_TEST(a, b, op, expected_result) { \
        Object result=operator(E, (a), (b), op); \
        assert_equal(E, result, expected_result); \
        dereference(E, &result); \
    }
    #define TYPED_OPERATOR_TEST(converting_function, a, b, op, expected_result) \
        OPERATOR_TEST(to_int(a), to_int(b), op, to_int(expected_result))
    #define INT_OPERATOR_TEST(a, b, op, expected_result) \
        OPERATOR_TEST(to_int(a), to_int(b), op, to_int(expected_result))
    #define STRING_OPERATOR_TEST(a, b, op, expected_result) \
        OPERATOR_TEST(to_string(a), to_string(b), op, to_string(expected_result))
    #define FLOAT_OPERATOR_TEST(a, b, op, expected_result) \
        OPERATOR_TEST(to_float(a), to_float(b), op, to_float(expected_result))

    #define OPERATOR_ERROR_TEST(a, b, op) { \
        Object result=operator(E, (a), (b), op); \
        assert(is_error(E, result)); \
        error_handle(E, result); \
        dereference(E, &result); \
    }

    INT_OPERATOR_TEST(2, 3, "+", 5)
    INT_OPERATOR_TEST(2, 3, "-", -1)
    OPERATOR_TEST(null_const, to_int(1), "-", to_int(-1))
    INT_OPERATOR_TEST(2, 3, "*", 6)
    OPERATOR_TEST(to_int(2), to_int(3), "/", to_float((float)2/3))
    INT_OPERATOR_TEST(2, 3, "//", 0)
    INT_OPERATOR_TEST(2, 3, "%", 2)

    FLOAT_OPERATOR_TEST(2, 3, "+", 5)
    FLOAT_OPERATOR_TEST(2, 3, "-", -1)
    FLOAT_OPERATOR_TEST(2, 3, "*", 6)
    FLOAT_OPERATOR_TEST(2, 3, "/", (float)2/3)
    OPERATOR_TEST(null_const, to_float(1.5), "-", to_float(-1.5))
    OPERATOR_ERROR_TEST(to_float(2), to_float(3), "%")
    OPERATOR_ERROR_TEST(to_float(2), to_float(3), "//")

    STRING_OPERATOR_TEST("foo", "bar", "+", "foobar")
    OPERATOR_TEST(to_string("foo"), to_int(3), "*", to_string("foofoofoo"))
    OPERATOR_TEST(to_string("foo"), to_int(3), "+", to_string("foo3"))
    OPERATOR_ERROR_TEST(to_int(3), to_string("foo"), "+")

    
    #define TRUE to_int(1)
    #define FALSE null_const
    #define BOOLEAN_OPERATOR(a, b, op, expected_result) { \
        Object result=operator(E, (a), (b), op); \
        assert(is_truthy(result)==expected_result); \
        dereference(E, &result); \
    }
    BOOLEAN_OPERATOR(TRUE, FALSE, "||", true)
    BOOLEAN_OPERATOR(TRUE, FALSE, "&&", false)
    BOOLEAN_OPERATOR(FALSE, TRUE, "!", false)

    #define OPERATOR_TEST(a, b, op, expected_result) { \
        Object result=operator(E, (a), (b), op); \
        assert_equal(E, result, expected_result); \
        dereference(E, &result); \
    }
    #undef TYPED_OPERATOR_TEST
    #undef INT_OPERATOR_TEST
    #undef STRING_OPERATOR_TEST
    #undef FLOAT_OPERATOR_TEST
    #undef OPERATOR_ERROR_TEST
    #undef TRUE
    #undef FALSE
    #undef BOOLEAN_OPERATOR
}

Object add_three(Executor* E, Object scope, Object* arguments, int arguments_count){
    assert(arguments_count==1);
    return operator(E, arguments[0], to_int(3), "+");
}

void function_calling(Executor* E){// tests whether f(5)==8 where f(x)=x+3
    Object f;
    function_init(E, &f);
    f.fp->native_pointer=add_three;
    f.fp->ftype=f_native;
    f.fp->arguments_count=1;

    assert_equal(E, call(E, f, OBJECTS_ARRAY(to_int(5)), 1), to_int(8));

    dereference(E, &f);
}

void table_indexing(Executor* E){// t["name"]="John" => t["name"]=="John"
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

    dereference(E, &t);
}

typedef Object (*ObjectGenerator)(Executor*, int);

Object ints_generator(Executor* E, int index){
    return to_int(index);
}

Object strings_generator(Executor* E, int index){
    return cast(E, to_int(index), t_string);
}

Object functions_generator(Executor* E, int index){
    return to_native_function(E, (ObjectSystemFunction)(long long)index, NULL, 0, false);
}

void fill_table(Executor* E, Table* table, 
                ObjectGenerator keys_generator, 
                ObjectGenerator values_generator,
                uint fields_count){
    for(int i=0; i<fields_count; i++){
        Object key=keys_generator(E, i);
        Object value=values_generator(E, i);
        table_set(E, table, key, value);
        dereference(E, &key);
        dereference(E, &value);
    }
}

void test_table(Executor* E, Table* table, 
                ObjectGenerator keys_generator, 
                ObjectGenerator values_generator,
                uint fields_count
){
    fill_table(E, table, keys_generator, values_generator, fields_count);
    for(int i=0; i<fields_count; i++){
        Object key=keys_generator(E, i);
        table_set(E, table, key, null_const);
        dereference(E, &key);
    }
    // assert(table->elements_count==0);
}

void sparse_array(Executor* E){
    Object table;
    table_init(E, &table);
    uint elements_count=100;
    uint max_index=100;
    uint seed=time(NULL);
    srand(seed);

    for(int i=0; i<elements_count; i++){
        Object key=to_int(rand()%max_index);
        Object value=functions_generator(E, i);
        table_set(E, table.tp, key, value);
        dereference(E, &key);
        dereference(E, &value);
    }
    
    srand(seed);
    for(int i=0; i<elements_count; i++){
        Object key=to_int(rand()%max_index);
        table_set(E, table.tp, key, null_const);
        dereference(E, &key);
    }
    // assert(table.tp->elements_count==0);
    dereference(E, &table);
}

Object return_scope(Executor* E, Object scope, Object* arguments, int arguments_count){
    return scope;
}

Object function_returning_constant(Executor* E, Object constant){
    Object function;
    function_init(E, &function);
    function.fp->variadic=true;// to ignore incorrect arguments count error
    function.fp->enclosing_scope=constant;
    reference(&function.fp->enclosing_scope);
    function.fp->native_pointer=return_scope;
    return function;
}

void test_table_overrides(Executor* E){
    Object table;
    table_init(E, &table);
    #define OVERRIDE_TEST(overriden_symbol, override_action) { \
        table_set(E, table.tp, overriden_symbol, \
                  function_returning_constant(E, overriden_symbol)); \
        Object action_result=override_action; \
        assert_equal(E, action_result, overriden_symbol); \
        dereference(E, &action_result); \
        table_set(E, table.tp, overriden_symbol, null_const); \
    }
    OVERRIDE_TEST(OVERRIDE(E, get), get(E, table, null_const))
    OVERRIDE_TEST(OVERRIDE(E, set), set(E, table, null_const, null_const))
    OVERRIDE_TEST(OVERRIDE(E, iterator), get_iterator(E, table))
    OVERRIDE_TEST(OVERRIDE(E, call), call(E, table, NULL, 0))
    OVERRIDE_TEST(OVERRIDE(E, cast), cast(E, table, t_string))
    OVERRIDE_TEST(OVERRIDE(E, cast), cast(E, table, t_string))
}

void table_resizing(Executor* E){
    Object table;
    table_init(E, &table);
    test_table(E, table.tp, ints_generator, functions_generator, 100);
    test_table(E, table.tp, strings_generator, functions_generator, 100);
    test_table(E, table.tp, functions_generator, functions_generator, 100);
    dereference(E, &table);
}

void table_iteration(Executor* E){
    uint fields_count=100;
    Object table;
    table_init(E, &table);
    fill_table(E, table.tp, ints_generator, functions_generator, fields_count);
    TableIterator it=table_get_iterator(table.tp);
    int sum=0;
    for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
        assert(i.key.type==t_int);
        sum+=i.key.int_value;
        assert(i.value.type==t_function);
    }
    // sum is a sum of 0...(fields_count-1)
    assert(sum==(fields_count-1)*fields_count/2);
    dereference(E, &table);
}

int main(){
    Executor E;
    object_system_init(&E);

    #define TEST(function) \
        printf("TEST: " #function "\n"); \
        function(&E); \
        printf("test successful\n");
    
    TRY_CATCH(
        // TODO test error objects
        TEST(operators)
        TEST(function_calling)
        TEST(table_indexing)
        TEST(table_resizing)
        TEST(sparse_array)
        TEST(table_iteration)
        TEST(test_table_overrides)
    ,
        printf("%s", err_message);
        exit(-1);
    )
    object_system_deinit(&E);
}