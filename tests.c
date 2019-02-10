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
    assert(compare(evaluate_string("1", false), to_number(1))==0);
    assert(compare(evaluate_string("1", true), to_number(1))==0);
    printf("test successful\n");
}

typedef struct {
    int a;
    float b;
    char* c;
} example_struct;

object example_struct_descriptor(example_struct* st){
    object class;
    table_init(&class);
    set(class, to_string("a"), to_field((int)&st->a-(int)st, n_int));
    set(class, to_string("b"), to_field((int)&st->b-(int)st, n_float));
    set(class, to_string("c"), to_field((int)&st->c-(int)st, n_string));
    return new_struct_descriptor(st, class);
}

bool float_equals(float a, float b, float epsilon){
  return fabs(a - b) < epsilon;
}

void struct_descriptor_tests(){
    example_struct* st=malloc(sizeof(example_struct));
    st->a=12;
    st->b=0.5;
    st->c=strdup("hello struct");
    object sd=example_struct_descriptor(st);

    assert(get(sd, to_string("a")).type==t_number);
    assert(get(sd, to_string("a")).value==12);
    set(sd, to_string("a"), to_number(10));
    assert(get(sd, to_string("a")).value==10);

    assert(get(sd, to_string("b")).type==t_number);
    assert(float_equals(get(sd, to_string("b")).value, 0.5, 0.01));
    set(sd, to_string("b"), to_number(0.1));
    assert(float_equals(get(sd, to_string("b")).value, 0.1, 0.01));

    assert(get(sd, to_string("c")).type==t_string);
    assert(strcmp(get(sd, to_string("c")).text, "hello struct")==0);
    set(sd, to_string("c"), to_string("test"));
    assert(strcmp(get(sd, to_string("c")).text, "test")==0);
}

int main(){
    table_init(&patching_table);
    TRY_CATCH(
        evaluation_tests();
        path_length_test();
        struct_descriptor_tests();
    ,
        printf(err_message);
        exit(-1);
    )
}
