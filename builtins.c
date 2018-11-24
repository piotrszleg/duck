#include "builtins.h"

// creates string variable str, executes body and frees the string afterwards
#define USING_STRING(string_expression, body) { char* str=string_expression; body; free(str); }

object* builtin_print(object* o, table* scope){
    USING_STRING(stringify(get((object*)scope, "self")),
        printf("%s\n", str));
    return (object*)new_null();
}

object* builtin_assert(object* o, table* scope){
    object* self=get((object*)scope, "self");
    if(is_falsy(self)){
        USING_STRING(stringify(self), 
            ERROR(MEMORY_ALLOCATION_FAILURE, "Assertion failed, %s is falsy.", str));
    }
    return (object*)new_null();
}

void register_builtins(table* scope){
    function* print_function=new_function();
    vector_add(&print_function->argument_names, strdup("self"));
    print_function->pointer=&builtin_print;

    set((object*)scope, "print", (object*)print_function);

    function* assert_function=new_function();
    vector_add(&assert_function->argument_names, strdup("self"));
    assert_function->pointer=&builtin_assert;

    set((object*)scope, "assert", (object*)assert_function);
}