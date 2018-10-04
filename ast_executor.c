#include "ast_executor.h"


/*
So what you're going to do is:
- add arguments vector to call function
- in ast_function_call get argument names from ast_function stored in data pointer of the function
- copy scope to function_scope
- add arguments to function_scope using names from ast_function vector and
*/

object* ast_function_call(object* o, table* scope){
    function* as_function=(function*)o;
    return execute_ast((expression*)as_function->data, scope);
}

object* native_print(object* o, table* scope){
    printf("%s\n", stringify(get((object*)scope, "self")));
    return (object*)new_null();
}

void register_globals(table* scope){
    function* print_function=new_function();
    vector_add(&print_function->argument_names, "self");
    print_function->pointer=&native_print;
    set((object*)scope, "print", (object*)print_function);
}

void copy_table(table* source, table* destination){
    const char *key;
    map_iter_t iter = map_iter(&source->fields);
    while (key = map_next(&source->fields, &iter)) {
        map_set(&destination->fields, key, (*map_get(&source->fields, key)));// dereference contained object, so it can be garbage collected
    }
}

object* execute_ast(expression* exp, table* scope){
    if(exp==NULL){
        ERROR(INCORRECT_OBJECT_POINTER, "AST expression pointer is null.");
    }
    object* result=NULL;
    switch(exp->type){
        case _literal:
        {
            literal* l=(literal*)exp;
            switch(l->ltype){
                case _int:
                    result=(object*)new_number();
                    ((number*)result)->value=l->ival;
                    break;
                case _float:
                    result=(object*)new_number();
                    ((number*)result)->value=l->fval;
                    break;
                case _string:
                    result=(object*)new_string();
                    ((string*)result)->value=l->sval;
                    break;
            }
            break;
        }
        case _block:
        {
            block* b=(block*)exp;
            table* new_scope=new_table();//TODO: scope inheritance
            register_globals(new_scope);
            for (int i = 0; i < vector_total(&b->lines); i++){
                result=execute_ast(vector_get(&b->lines, i), scope);
            }
            if(b->is_table){
                result=(object*)scope;
            }
            break;
        }
        case _name:
        {
            result=get((object*)scope, ((name*)exp)->value);
            break;
        }
        case _assignment:
        {
            assignment* a=(assignment*)exp;
            result=execute_ast(a->right, scope);
            set((object*)scope, a->left->value, result);
            break;
        }
        case _unary:
        {
            unary* u=(unary*)exp;
            result=operator(execute_ast(u->left, scope), execute_ast(u->right, scope), u->op);
            break;
        }
        case _conditional:
        {
            conditional* c=(conditional*)exp;
            if(is_falsy(execute_ast(c->condition, scope))){
                result=execute_ast((expression*)c->onfalse, scope);
            } else{
                result=execute_ast((expression*)c->ontrue, scope);
            }
            break;
        }
        case _function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;
            function* f=(function*)new_function();
            for (int i = 0; i < vector_total(d->arguments); i++){
                vector_add(&f->argument_names, ((name*)vector_get(d->arguments, i))->value);
            }
            f->data=(void*)d->body;
            f->enclosing_scope=scope;
            f->pointer=ast_function_call;
            result=(object*)f;
            break;
        }
        case _function_call:
        {
            function_call* c=(function_call*)exp;
            table* function_scope=new_table();
            function* f=(function*)get((object*)scope, c->function_name->value);
            if(f->type==t_null){
                ERROR(INCORRECT_OBJECT_POINTER, "No function named %s in the current scope.", c->function_name->value);
            }
            for (int i = 0; i < vector_total(&c->arguments->lines); i++){
                char buf[16];
                itoa(i, buf, 10);
                set((object*)function_scope, vector_get(&f->argument_names, i), execute_ast(vector_get(&c->arguments->lines, i), scope));// here instead of "b" should be argument name from function decalaration
            }
            result=(object*)call((object*)f, function_scope);
            break;
        }
        default:
        {
            printf("uncatched type: %i\n", exp->type);
            result=(object*)new_null();
        }
    }
    return result;
}