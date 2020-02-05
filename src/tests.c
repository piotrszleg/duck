#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include "bytecode_optimisations/bytecode_optimisations.h"
#include "runtime/struct_descriptor.h"
#include "execution/execution.h"
#include "utility.h"

void evaluation_tests(Executor* E){
    printf("TEST: %s\n", __FUNCTION__);
    assert(compare(E, evaluate_string(E, "1", null_const), to_int(1))==0);

    printf("test successful\n");
}

typedef struct {
    int a;
    float b;
    char* c;
} ExampleStruct;


#define OFFSET(structure, field) (long long)&(structure).field-(long long)&(structure)
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
    st.c=strdup("hello struct");
    Object sd=new_struct_descriptor(E, &st, example_struct_class(E));

    assert(get(E, sd, to_string("a")).type==t_int);
    assert(get(E, sd, to_string("a")).int_value==12);
    set(E, sd, to_string("a"), to_int(10));
    assert(st.a==10);
    assert(get(E, sd, to_string("a")).int_value==10);

    assert(get(E, sd, to_string("b")).type==t_float);
    assert(float_equals(get(E, sd, to_string("b")).float_value, 0.5, 0.01));
    set(E, sd, to_string("b"), to_float(0.1));
    assert(float_equals(st.b, 0.1, 0.01));
    assert(float_equals(get(E, sd, to_string("b")).float_value, 0.1, 0.01));

    assert(get(E, sd, to_string("c")).type==t_string);
    assert(strcmp(get(E, sd, to_string("c")).text, "hello struct")==0);
    set(E, sd, to_string("c"), to_string("test"));
    assert(strcmp(st.c, "test")==0);
    assert(strcmp(get(E, sd, to_string("c")).text, "test")==0);

    dereference(E, &sd);
    // struct descriptors by deafult don't contain destructors
    free(st.c);

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
    assert(st->a.a==32);
    Object substructure_a=get(E, substructure_pointer, to_string("a"));
    assert(substructure_a.type==t_int);
    assert(substructure_a.int_value==32);
    assert(float_equals(st->a.b, 0.1, 0.01));
    Object substructure_b=get(E, substructure_pointer, to_string("b"));
    assert(substructure_b.type==t_float);
    assert(float_equals(substructure_b.float_value, 0.1, 0.01));
    assert(strcmp(st->a.c, "test test")==0);
    Object substructure_c=get(E, substructure_pointer, to_string("c"));
    assert(substructure_c.type==t_string);
    assert(strcmp(substructure_c.text, "test test")==0);
}

#define ASSERT_OBJECTS_EQUAL(a, b) \
    {Object temp_a=a; \
     Object temp_b=b; \
     assert(compare(E, temp_a, temp_b)==0); \
     dereference(E, &temp_a); \
     dereference(E, &temp_b);}

void struct_descriptor_nested_tests(Executor* E){
    printf("TEST: %s\n", __FUNCTION__);
    ExampleStructNested st;
    ExampleStruct contained={10, 0.75, strdup("test")};
    
    st.a=contained;
    st.b=&contained;
    st.c=&contained.a;
    Object sd=new_struct_descriptor(E, &st, example_struct_nested_class(E));

    Object contained_replacement;
    table_init(E, &contained_replacement);
    set(E, contained_replacement, to_string("a"), to_int(32));
    set(E, contained_replacement, to_string("b"), to_float(0.1));
    set(E, contained_replacement, to_string("c"), to_string("test test"));

    Object substructure_pointer;

    reference(&contained_replacement);
    set(E, sd, to_string("a"), contained_replacement);
    substructure_pointer=get(E, sd, to_string("a"));
    test_substructure(E, &st, substructure_pointer);

    set(E, sd, to_string("b"), contained_replacement);
    substructure_pointer=get(E, sd, to_string("b"));
    test_substructure(E, &st, substructure_pointer);
    dereference(E, &substructure_pointer);
    dereference(E, &contained_replacement);

    Object c_pointer=get(E, sd, to_string("c"));
    ASSERT_OBJECTS_EQUAL(get(E, c_pointer, to_int(0)), to_int(32));
    set(E, c_pointer, to_int(0), to_int(16));
    ASSERT_OBJECTS_EQUAL(get(E, c_pointer, to_int(0)), to_int(16));
    
    Object scope;
    table_init(E, &scope);
    set(E, scope, to_string("sd"), sd);
    set(E, scope, to_string("replacement"), sd);

    evaluate_string(E, "global.float_close=(a, b)->{diff=a-b, (diff<0.1 && diff>-0.1)}", scope);

    #define ASSERT_EVAL(expression, ...) \
        {char* formatted=suprintf(expression, ##__VA_ARGS__); \
        Object evaluation_result=evaluate_string(E, formatted, scope); \
        free(formatted); \
        assert(is_truthy(evaluation_result));}
    
    ASSERT_EVAL("sd.a.a==%i", st.a.a);
    ASSERT_EVAL("float_close(sd.a.b, %f)", st.a.b);
    ASSERT_EVAL("sd.a.c==\"%s\"", st.a.c);
    set(E, sd, to_string("a"), contained_replacement);
    ASSERT_EVAL("sd.a.a==replacement.a");
    ASSERT_EVAL("global.float_eq(sd.a.b, replacement.b)");
    ASSERT_EVAL("sd.a.c==replacement.c");

    ASSERT_EVAL("sd.b.a==%i", st.b->a);
    ASSERT_EVAL("float_close(sd.b.b, %f)", st.b->b);
    ASSERT_EVAL("st.b.c==\"%s\"", st.b->c);
    set(E, sd, to_string("b"), contained_replacement);
    ASSERT_EVAL("sd.b.a==replacement.a");
    ASSERT_EVAL("float_close(sd.b.b, replacement.b)");
    ASSERT_EVAL("sd.b.c==replacement.c");

    free(contained.c);

    printf("test successful\n");
}

void string_replace_multiple_test(){
    ReplacementPair pairs[]={
        CONSTANT_REPLACEMENT_PAIR("doge", "wolfe"),
        CONSTANT_REPLACEMENT_PAIR("cat", "tiger"),
        CONSTANT_REPLACEMENT_PAIR("dog", "wolf"),
        CONSTANT_REPLACEMENT_PAIR("Cat", "Tiger"),
        CONSTANT_REPLACEMENT_PAIR("Dog", "Wolf"),
    };
    #define TEST_REPLACE(original, expected) \
    {   char* after_replace=string_replace_multiple(original, pairs, 5); \
        assert(strcmp(after_replace, expected)==0); \
        free(after_replace); }

    TEST_REPLACE("A white cat jumped over a black dog and the dog barked at the cat.", "A white tiger jumped over a black wolf and the wolf barked at the tiger.");
    TEST_REPLACE("A white cat jumped over a black doge and the doge barked at the cat.", "A white tiger jumped over a black wolfe and the wolfe barked at the tiger.");
    TEST_REPLACE("Cat jumped over a black dog and the dog barked at the cat", "Tiger jumped over a black wolf and the wolf barked at the tiger");
}

int main(){
    Executor E;
    executor_init(&E);
    TRY_CATCH(
        vector_tests();
        evaluation_tests(&E);
        struct_descriptor_tests(&E);
        struct_descriptor_nested_tests(&E);
        string_replace_multiple_test();
    ,
        printf("%s", err_message);
        exit(-1);
    )
    executor_deinit(&E);
}

#undef FIELD
#undef OFFSET