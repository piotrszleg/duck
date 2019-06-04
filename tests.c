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
    ASSERT_PATH_LENGTH(((instruction[]){
        {b_load_string, 0},
        {b_get, 0}
    }), 1, 2)
    // counting starts from load_string instruction so path length should be zero
    ASSERT_PATH_LENGTH(((instruction[]){
        {b_load_string, 0},
        {b_get, 0}
    }), 0, 0)
    // no path present
    ASSERT_PATH_LENGTH(((instruction[]){
        {b_double, 0},
        {b_jump, 0}
    }), 1, 0)
    // path of length 4
    ASSERT_PATH_LENGTH(((instruction[]){
        {b_load_string, 0},
        {b_get, 0},
        {b_load_string, 0},
        {b_get, 1}
    }), 3, 4)
    // path of length 6
    ASSERT_PATH_LENGTH(((instruction[]){
        {b_load_string, 0},
        {b_get, 0},
        {b_load_string, 0},
        {b_get, 0},
        {b_load_string, 0},
        {b_get, 1}
    }), 5, 6)
    // path of length 4 and random instructions after it
    ASSERT_PATH_LENGTH(((instruction[]){
        {b_double, 0},
        {b_jump, 0},
        {b_load_string, 0},
        {b_get, 0},
        {b_load_string, 0},
        {b_get, 1}
    }), 5, 4)

    printf("test successful\n");
}

void evaluation_tests(){
    printf("TEST: %s\n", __FUNCTION__);
    assert(compare(evaluate_string("1", null_const), to_number(1))==0);

    printf("test successful\n");
}

typedef struct {
    int a;
    float b;
    char* c;
} example_struct;

#define OFFSET(structure, field) (int)&(structure).field-(int)&(structure)
#define FIELD(class, structure, field, type) set(class, to_string(#field), to_field(OFFSET(structure, field), type));

object example_struct_class() {
    example_struct st;
    object class;
    table_init(&class);
    
    FIELD(class, st, a, n_int)
    FIELD(class, st, b, n_float)
    FIELD(class, st, c, n_string)

    return class;
}

bool float_equals(float a, float b, float epsilon){
  return fabs(a - b) < epsilon;
}

void struct_descriptor_tests(){
    printf("TEST: %s\n", __FUNCTION__);
    example_struct st;
    st.a=12;
    st.b=0.5;
    st.c="hello struct";
    object sd=new_struct_descriptor(&st, example_struct_class());

    assert(get(sd, to_string("a")).type==t_number);
    assert(get(sd, to_string("a")).value==12);
    set(sd, to_string("a"), to_number(10));
    assert(st.a==10);
    assert(get(sd, to_string("a")).value==10);

    assert(get(sd, to_string("b")).type==t_number);
    assert(float_equals(get(sd, to_string("b")).value, 0.5, 0.01));
    set(sd, to_string("b"), to_number(0.1));
    assert(float_equals(st.b, 0.1, 0.01));
    assert(float_equals(get(sd, to_string("b")).value, 0.1, 0.01));

    assert(get(sd, to_string("c")).type==t_string);
    assert(strcmp(get(sd, to_string("c")).text, "hello struct")==0);
    set(sd, to_string("c"), to_string("test"));
    assert(strcmp(st.c, "test")==0);
    assert(strcmp(get(sd, to_string("c")).text, "test")==0);

    printf("test successful\n");
}

typedef struct {
    example_struct a;
    example_struct* b;
    int* c;
} example_struct_nested;

object to_struct_field(int offset, object class){
    object struct_field=to_field(offset, n_struct);
    set(struct_field, to_string("class"), class);
    return struct_field;
}

object example_struct_nested_class(){
    example_struct_nested st;
    object class;
    table_init(&class);

    set(class, to_string("a"), to_struct_field(OFFSET(st, a), example_struct_class()));

    object b_field=to_field(OFFSET(st, b), n_pointer);
    set(b_field, to_string("pointed"), to_struct_field(0, example_struct_class()));
    set(class, to_string("b"), b_field);

    object c_field=to_field(OFFSET(st, c), n_pointer);
    set(c_field, to_string("pointed"), to_field(0, n_int));
    set(class, to_string("c"), c_field);

    return class;
}

void test_substructure(example_struct_nested* st, object substructure_pointer){
    assert(st->a.a=32);
    object substructure_a=get(substructure_pointer, to_string("a"));
    assert(substructure_a.type==t_number);
    assert(substructure_a.value==32);
    assert(float_equals(st->a.b, 0.1, 0.01));
    object substructure_b=get(substructure_pointer, to_string("b"));
    assert(substructure_b.type==t_number);
    assert(float_equals(substructure_b.value, 0.1, 0.01));
    assert(strcmp(st->a.c, "test test")==0);
    object substructure_c=get(substructure_pointer, to_string("c"));
    assert(substructure_c.type==t_string);
    assert(strcmp(substructure_c.text, "test test")==0);
}

#define ASSERT_EVAL(expression, scope) \
    {object evaluation_result=evaluate_string(expression, scope); \
    assert(!is_falsy(evaluation_result));}

void struct_descriptor_nested_tests(){
    printf("TEST: %s\n", __FUNCTION__);
    example_struct_nested* st=malloc(sizeof(example_struct_nested));
    example_struct contained={10, 0.75, "test"};
    
    st->a=contained;
    st->b=&contained;
    st->c=&contained.a;
    object sd=new_struct_descriptor(st, example_struct_nested_class());

    object contained_replacement;
    table_init(&contained_replacement);
    set(contained_replacement, to_string("a"), to_number(32));
    set(contained_replacement, to_string("b"), to_number(0.1));
    set(contained_replacement, to_string("c"), to_string("test test"));

    object substructure_pointer;

    reference(&contained_replacement);
    set(sd, to_string("a"), contained_replacement);
    substructure_pointer=get(sd, to_string("a"));
    test_substructure(st, substructure_pointer);

    set(sd, to_string("b"), contained_replacement);
    substructure_pointer=get(sd, to_string("b"));
    test_substructure(st, substructure_pointer);
    dereference(&substructure_pointer);
    dereference(&contained_replacement);

    object c_pointer=get(sd, to_string("c"));
    assert(compare(get(c_pointer, to_number(0)), to_number(32))==0);
    set(c_pointer, to_number(0), to_number(16));
    assert(compare(get(c_pointer, to_number(0)), to_number(16))==0);

    /*
    // TODO reimplement it in the language
    object scope;
    table_init(&scope);
    set(scope, to_string("sd"), sd);
    set(scope, to_string("replacement"), sd);

    evaluate_string("global.float_eq=(a, b)->{diff=a-b, (diff<0.1 && diff>-0.1)}", scope);

    ASSERT_EVAL("sd.a.a==10", scope);
    ASSERT_EVAL("global.float_eq(sd.a.b, 0.75)", scope);
    printf("<%s>", stringify(evaluate_string("sd.a", scope)));
    ASSERT_EVAL("sd.a.c==\"test\"", scope);
    set(sd, to_string("a"), contained_replacement);
    ASSERT_EVAL("sd.a.a==replacement.a", scope);
    ASSERT_EVAL("global.float_eq(sd.a.b, replacement.b)", scope);
    ASSERT_EVAL("sd.a.c==replacement.c", scope);

    ASSERT_EVAL("sd.b.a==10", scope);
    ASSERT_EVAL("sd.b.b==0.75", scope);
    ASSERT_EVAL("st.b.c==\"test\"", scope);
    set(sd, to_string("b"), contained_replacement);
    ASSERT_EVAL("sd.b.a==replacement.a", scope);
    ASSERT_EVAL("sd.b.b==replacement.b", scope);
    ASSERT_EVAL("sd.b.c==replacement.c", scope);
    */

    printf("test successful\n");
}

int main(){
    table_init(&patching_table);
    TRY_CATCH(
        evaluation_tests();
        path_length_test();
        struct_descriptor_tests();
        struct_descriptor_nested_tests();
    ,
        printf(err_message);
        exit(-1);
    )
}

#undef FIELD
#undef OFFSET