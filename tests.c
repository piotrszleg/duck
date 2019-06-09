#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include "optimisations/bytecode_optimisations.h"
#include "runtime/struct_descriptor.h"
#include "execution.h"

void path_length_test(){// tests whether "count: "+5="count: 5"
    printf("TEST: %s\n", __FUNCTION__);

    #define ASSERT_PATH_LENGTH(code, path_start, expected_length) \
        assert(path_length(code, path_start)==expected_length);

    // correct path starting from b_get
    ASSERT_PATH_LENGTH(((Instruction[]){
        {b_load_string, 0},
        {b_get, 0}
    }), 1, 2)
    // counting starts from load_string Instruction so path length should be zero
    ASSERT_PATH_LENGTH(((Instruction[]){
        {b_load_string, 0},
        {b_get, 0}
    }), 0, 0)
    // no path present
    ASSERT_PATH_LENGTH(((Instruction[]){
        {b_double, 0},
        {b_jump, 0}
    }), 1, 0)
    // path of length 4
    ASSERT_PATH_LENGTH(((Instruction[]){
        {b_load_string, 0},
        {b_get, 0},
        {b_load_string, 0},
        {b_get, 1}
    }), 3, 4)
    // path of length 6
    ASSERT_PATH_LENGTH(((Instruction[]){
        {b_load_string, 0},
        {b_get, 0},
        {b_load_string, 0},
        {b_get, 0},
        {b_load_string, 0},
        {b_get, 1}
    }), 5, 6)
    // path of length 4 and random instructions after it
    ASSERT_PATH_LENGTH(((Instruction[]){
        {b_double, 0},
        {b_jump, 0},
        {b_load_string, 0},
        {b_get, 0},
        {b_load_string, 0},
        {b_get, 1}
    }), 5, 4)

    printf("test successful\n");
}

void evaluation_tests(Executor* E){
    printf("TEST: %s\n", __FUNCTION__);
    assert(compare(evaluate_string(E, "1", null_const), to_number(1))==0);

    printf("test successful\n");
}

typedef struct {
    int a;
    float b;
    char* c;
} ExampleStruct;

#define OFFSET(structure, field) (int)&(structure).field-(int)&(structure)
#define FIELD(class, structure, field, type) set(E, class, to_string(#field), to_field(E, OFFSET(structure, field), type));

Object example_struct_class(Executor* E) {
    ExampleStruct st;
    Object class;
    table_init(E, &class);
    
    FIELD(class, st, a, n_int)
    FIELD(class, st, b, n_float)
    FIELD(class, st, c, n_string)

    return class;
}

bool float_equals(float a, float b, float epsilon){
  return fabs(a - b) < epsilon;
}

void struct_descriptor_tests(Executor* E){
    printf("TEST: %s\n", __FUNCTION__);
    ExampleStruct st;
    st.a=12;
    st.b=0.5;
    st.c="hello struct";
    Object sd=new_struct_descriptor(E, &st, example_struct_class(E));

    assert(get(E, sd, to_string("a")).type==t_number);
    assert(get(E, sd, to_string("a")).value==12);
    set(E, sd, to_string("a"), to_number(10));
    assert(st.a==10);
    assert(get(E, sd, to_string("a")).value==10);

    assert(get(E, sd, to_string("b")).type==t_number);
    assert(float_equals(get(E, sd, to_string("b")).value, 0.5, 0.01));
    set(E, sd, to_string("b"), to_number(0.1));
    assert(float_equals(st.b, 0.1, 0.01));
    assert(float_equals(get(E, sd, to_string("b")).value, 0.1, 0.01));

    assert(get(E, sd, to_string("c")).type==t_string);
    assert(strcmp(get(E, sd, to_string("c")).text, "hello struct")==0);
    set(E, sd, to_string("c"), to_string("test"));
    assert(strcmp(st.c, "test")==0);
    assert(strcmp(get(E, sd, to_string("c")).text, "test")==0);

    printf("test successful\n");
}

typedef struct {
    ExampleStruct a;
    ExampleStruct* b;
    int* c;
} ExampleStructNested;

Object example_struct_nested_class(Executor* E){
    ExampleStructNested st;
    Object class;
    table_init(E, &class);

    set(E, class, to_string("a"), to_struct_field(E, OFFSET(st, a), example_struct_class(E)));
    set(E, class, to_string("b"), to_struct_pointer_field(E, OFFSET(st, b), example_struct_class(E)));

    Object c_field=to_field(E, OFFSET(st, c), n_pointer);
    set(E, c_field, to_string("pointed"), to_field(E, 0, n_int));
    set(E, class, to_string("c"), c_field);

    return class;
}

void test_substructure(Executor* E, ExampleStructNested* st, Object substructure_pointer){
    assert(st->a.a=32);
    Object substructure_a=get(E, substructure_pointer, to_string("a"));
    assert(substructure_a.type==t_number);
    assert(substructure_a.value==32);
    assert(float_equals(st->a.b, 0.1, 0.01));
    Object substructure_b=get(E, substructure_pointer, to_string("b"));
    assert(substructure_b.type==t_number);
    assert(float_equals(substructure_b.value, 0.1, 0.01));
    assert(strcmp(st->a.c, "test test")==0);
    Object substructure_c=get(E, substructure_pointer, to_string("c"));
    assert(substructure_c.type==t_string);
    assert(strcmp(substructure_c.text, "test test")==0);
}

#define ASSERT_EVAL(expression, scope) \
    {Object evaluation_result=evaluate_string(expression, scope); \
    assert(!is_falsy(evaluation_result));}

void struct_descriptor_nested_tests(Executor* E){
    printf("TEST: %s\n", __FUNCTION__);
    ExampleStructNested* st=malloc(sizeof(ExampleStructNested));
    ExampleStruct contained={10, 0.75, "test"};
    
    st->a=contained;
    st->b=&contained;
    st->c=&contained.a;
    Object sd=new_struct_descriptor(E, st, example_struct_nested_class(E));

    Object contained_replacement;
    table_init(E, &contained_replacement);
    set(E, contained_replacement, to_string("a"), to_number(32));
    set(E, contained_replacement, to_string("b"), to_number(0.1));
    set(E, contained_replacement, to_string("c"), to_string("test test"));

    Object substructure_pointer;

    reference(&contained_replacement);
    set(E, sd, to_string("a"), contained_replacement);
    substructure_pointer=get(E, sd, to_string("a"));
    test_substructure(E, st, substructure_pointer);

    set(E, sd, to_string("b"), contained_replacement);
    substructure_pointer=get(E, sd, to_string("b"));
    test_substructure(E, st, substructure_pointer);
    dereference(E, &substructure_pointer);
    dereference(E, &contained_replacement);

    Object c_pointer=get(E, sd, to_string("c"));
    assert(compare(get(E, c_pointer, to_number(0)), to_number(32))==0);
    set(E, c_pointer, to_number(0), to_number(16));
    assert(compare(get(E, c_pointer, to_number(0)), to_number(16))==0);

    /*
    // TODO reimplement it in the language
    Object scope;
    table_init(E, &scope);
    set(E, scope, to_string("sd"), sd);
    set(E, scope, to_string("replacement"), sd);

    evaluate_string("global.float_eq=(a, b)->{diff=a-b, (diff<0.1 && diff>-0.1)}", scope);

    ASSERT_EVAL("sd.a.a==10", scope);
    ASSERT_EVAL("global.float_eq(sd.a.b, 0.75)", scope);
    printf("<%s>", stringify(evaluate_string("sd.a", scope)));
    ASSERT_EVAL("sd.a.c==\"test\"", scope);
    set(E, sd, to_string("a"), contained_replacement);
    ASSERT_EVAL("sd.a.a==replacement.a", scope);
    ASSERT_EVAL("global.float_eq(sd.a.b, replacement.b)", scope);
    ASSERT_EVAL("sd.a.c==replacement.c", scope);

    ASSERT_EVAL("sd.b.a==10", scope);
    ASSERT_EVAL("sd.b.b==0.75", scope);
    ASSERT_EVAL("st.b.c==\"test\"", scope);
    set(E, sd, to_string("b"), contained_replacement);
    ASSERT_EVAL("sd.b.a==replacement.a", scope);
    ASSERT_EVAL("sd.b.b==replacement.b", scope);
    ASSERT_EVAL("sd.b.c==replacement.c", scope);
    */

    printf("test successful\n");
}

int main(){
    Executor E;
    E.gc=malloc(sizeof(GarbageCollector));
    object_system_init(&E);
    E.options=default_options;
    TRY_CATCH(
        evaluation_tests(&E);
        path_length_test(&E);
        struct_descriptor_tests(&E);
        struct_descriptor_nested_tests(&E);
    ,
        printf(err_message);
        exit(-1);
    )
    object_system_deinit(&E);
    free(E.gc);
}

#undef FIELD
#undef OFFSET