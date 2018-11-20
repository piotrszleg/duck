#include "builtins.h"

// creates string variable str, executes body and frees the string afterwards
#define USING_STRING(string_expression, body) { char* str=string_expression; body; free(str); }

object* native_print(object* o, table* scope){
    USING_STRING(stringify(get((object*)scope, "self")),
        printf("%s\n", str));
    return (object*)new_null();
}

void register_builtins(table* scope){
    function* print_function=new_function();
    vector_add(&print_function->argument_names, "self");
    print_function->pointer=&native_print;

    set((object*)scope, "print", (object*)print_function);
}