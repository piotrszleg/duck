#include "ast_executor.h"

struct ast_function{
    vector arguments;
    block body;
};

object* execute_ast(expression* exp, table* scope);

object* ast_function_call(object* o, table* arguments){
    expression* function_body=(expression*)(((function*)o)->data);
    table* t=(table*)object_new(t_table);
    /*const char *key;
    map_iter_t iter = map_iter(&m);
    while ((key = map_next(&t->fields, &iter))) {
         *map_get(&t->fields, key)
    }*/
    return execute_ast(function_body, arguments);
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
                    result=object_new(t_number);
                    ((number*)result)->value=l->ival;
                    break;
                case _float:
                    result=object_new(t_number);
                    ((number*)result)->value=l->fval;
                    break;
                case _string:
                    result=object_new(t_string);
                    ((string*)result)->value=l->sval;
                    break;
            }
            break;
        }
        case _block:
        {
            block* b=(block*)exp;
            table* scope=(table*)object_new(t_table);
            for (int i = 0; i < vector_total(&b->lines); i++){
                execute_ast(vector_get(&b->lines, i), scope);
            }
            result=(object*)scope;
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
            switch(u->op){
                case '+':
                    result=add(execute_ast(u->left, scope), execute_ast(u->right, scope));
                    break;
                default:
                    printf("operator: %c\n", u->op);
                    result=object_new(t_null);
            }
            break;
        }
        case _function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;
            function* f=(function*)object_new(t_function);

            f->data=(void*)d->body;
            f->pointer=ast_function_call;
            result=(object*)f;
            break;
        }
        case _function_call:
        {
            function_call* c=(function_call*)exp;
            table* function_scope=(table*)object_new(t_table);
            for (int i = 0; i < vector_total(&c->arguments->lines); i++){
                char buf[16];
                itoa(i, buf, 10);
                set((object*)function_scope, "b", execute_ast(vector_get(&c->arguments->lines, i), scope));// here instead of "b" should be argument name from function decalaration
            }
            result=(object*)call(get((object*)scope, c->function_name->value), function_scope);
            break;
        }
        default:
        {
            printf("type: %i\n", exp->type);
            result=object_new(t_null);
        }
    }
    return result;
}