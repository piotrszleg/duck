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

object* scope_get_override(object* o, table* scope){
    object* self=get((object*)scope, "self");
    if(self->type!=t_table){
        ERROR(WRONG_ARGUMENT_TYPE, "Table get override incorrect self argument.");
        return (object*)new_null();
    }
    object* key=get((object*)scope, "key");
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