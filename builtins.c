#include "builtins.h"

// creates string variable str, executes body and frees the string afterwards
#define USING_STRING(string_expression, body) { char* str=string_expression; body; free(str); }

object* builtin_print(vector arguments){
    USING_STRING(stringify(vector_get(&arguments, 0)),
        printf("%s\n", str));
    return (object*)new_null();
}

object* builtin_assert(vector arguments){
    object* self=vector_get(&arguments, 0);
    if(is_falsy(self)){
        USING_STRING(stringify(self), 
            ERROR(MEMORY_ALLOCATION_FAILURE, "Assertion failed, %s is falsy.", str));
    }
    return (object*)new_null();
}

void register_builtins(table* scope){
    function* print_function=new_function();
    print_function->arguments_count=1;
    print_function->pointer=&builtin_print;

    set((object*)scope, "print", (object*)print_function);

    function* assert_function=new_function();
    assert_function->arguments_count=1;
    assert_function->pointer=&builtin_assert;

    set((object*)scope, "assert", (object*)assert_function);
}

object* scope_get_override(vector arguments){
    object* self=vector_get(&arguments, 0);
    if(self->type!=t_table){
        ERROR(WRONG_ARGUMENT_TYPE, "Table get override incorrect self argument.");
        return (object*)new_null();
    }
    object* key=vector_get(&arguments, 1);
    if(key->type!=t_string){
        ERROR(WRONG_ARGUMENT_TYPE, "Table get override incorrect key argument.");
        return (object*)new_null();
    }
    object** map_get_result=map_get(&((table*)self)->fields, ((string*)key)->value);

    if(map_get_result!=NULL){
        return *map_get_result;
    } else{
        object* base=get((object*)self, "base");
        if(base->type==t_table){
            return get(base, ((string*)key)->value);
        }
        return (object*)new_null();
    }
}

void setup_scope(object* scope, object* base){
    function* f=new_function();
    f->pointer=&scope_get_override;
    object* base_global=get(base, "global");
    if(base_global->type!=t_null){
        set(scope, "global", base_global);
    } else {
        set(scope, "global", base);
        object_delete(base_global);// base_global is null so it can be safely deleted
    }
    set(scope, "get", (object*)f);
    set(scope, "scope", scope);
    set(scope, "base", base);
}