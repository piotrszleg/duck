#include <assert.h>
#include "optimisations/bytecode_optimisations.h"

void path_length_test(){// tests whether "count: "+5="count: 5"
    printf("TEST: %s\n", __FUNCTION__);

    #define ASSERT_PATH_LENGTH(code, path_start, expected_length) \
        assert(path_length(code, path_start)==expected_length);

    ASSERT_PATH_LENGTH(((instruction[]){
        {b_load_string, 0},
        {b_get, 0}
    }), 1, 2)
    ASSERT_PATH_LENGTH(((instruction[]){
        {b_load_string, 0},
        {b_get, 0}
    }), 0, 0)
    ASSERT_PATH_LENGTH(((instruction[]){
        {b_double, 0},
        {b_jump, 0}
    }), 1, 0)

    printf("test successful\n");
}

int main(){
    TRY_CATCH(
        path_length_test();
    ,
        printf(err_message);
        exit(-1);
    )
}
