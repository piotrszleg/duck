#include "ast_executor.h"

int current_line;

// creates string variable str, executes body and frees the string afterwards
#define USING_STRING(string_expression, body) { char* str=string_expression; body; free(str); }

object* ast_function_call(object* o, table* scope){
    function* as_function=(function*)o;
    return execute_ast((expression*)as_function->data, scope, 1);
}

object* native_print(object* o, table* scope){
    USING_STRING(stringify(get((object*)scope, "self")),
        printf("%s\n", str));
    return (object*)new_null();
}

object* scope_get_override(object* o, table* scope){
    object* self=get((object*)scope, "self");
    if(self->type!=t_table){
        ERROR(WRONG_ARGUMENT_TYPE, "Table get override incorrect self argument.");
        return (object*)new_null();
    }
    object* key=((string*)get((object*)scope, "key"));
    if(key->type!=t_string){
        ERROR(WRONG_ARGUMENT_TYPE, "Table get override incorrect key argument.");
        return (object*)new_null();
    }
    object** map_get_result=map_get(&((table*)self)->fields, ((string*)key)->value);

    if(map_get_result!=NULL){
        return *map_get_result;
    } else{
        object* base=((string*)get((object*)self, "base"));
        if(base->type==t_table){
            return get(base, ((string*)key)->value);
        }
        return (object*)new_null();
    }
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
        map_set(&destination->fields, key, (*map_get(&source->fields, key)));
    }
}

object* path_get(table* scope, path p){
    table* current=scope;
    int lines_count=vector_total(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= vector_get(&p.lines, i);
        char* evaluated_to_string;
        if(e->type==_name){
            evaluated_to_string=((name*)e)->value;
        } else {
            evaluated_to_string=stringify(execute_ast(e, scope, 0));
        }
        object* object_at_name=get((object*)current, evaluated_to_string);
        if(i==lines_count-1){
            return object_at_name;
        } else if(object_at_name->type==t_table) {
            current=(table*)object_at_name;
        } else {
            ERROR(INCORRECT_OBJECT_POINTER, "%s is not a table.", stringify(object_at_name));
            return new_null();
        }
    }
    return new_null();
} 

void path_set(table* scope, path p, object* value){
    table* current=scope;
    int lines_count=vector_total(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= vector_get(&p.lines, i);
        char* evaluated_to_string;
        if(e->type==_name){
            evaluated_to_string=((name*)e)->value;
        } else {
            evaluated_to_string=stringify(execute_ast(e, scope, 0));
        }
        if(i==lines_count-1){
            set((object*)current, evaluated_to_string, value);
        } else{
            object* object_at_name=get(current, evaluated_to_string);
            if(object_at_name->type==t_table) {
                current=(table*)object_at_name;
            } else {
                ERROR(INCORRECT_OBJECT_POINTER, "%s is not a table.", stringify(object_at_name));
            }
        }
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
    }
    set(scope, "scope", scope);
    set(scope, "get", f);
    set(scope, "base", base);
}

object* execute_ast(expression* exp, table* scope, int keep_scope){
    if(exp==NULL){
        ERROR(INCORRECT_OBJECT_POINTER, "AST expression pointer is null.");
    }
    current_line=exp->line;
    object* result=NULL;
    switch(exp->type){
        case _empty:
            result=(object*)new_null();
            break;
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
        case _table_literal:
        {
            block* b=(block*)exp;
            table* new_scope=new_table();
            new_scope->ref_count++;
            for (int i = 0; i < vector_total(&b->lines); i++){
                expression* line=(expression*)vector_get(&b->lines, i);
                if(line->type==_assignment){
                    result=execute_ast(line, new_scope, 0);
                } else {
                    char buf[16];
                    sprintf(buf,"%d",i);
                    set(new_scope, buf, execute_ast(line, new_scope, 0));
                }
            }
            result=(object*)new_scope;
            break;
        }
        case _block:
        {
            block* b=(block*)exp;
            table* block_scope;
            if(keep_scope){
                block_scope=scope;
            } else {
                block_scope=new_table();
                block_scope->ref_count++;
                setup_scope((object*)block_scope, (object*)scope);
            }
            for (int i = 0; i < vector_total(&b->lines); i++){
                result=execute_ast(vector_get(&b->lines, i), block_scope, 0);
            }
            result->ref_count++;
            if(!keep_scope){
                object_delete((object*)block_scope);
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
            result=execute_ast(a->right, scope, 0);
            path_set(scope, *a->left, result);
            break;
        }
        case _unary:
        {
            unary* u=(unary*)exp;
            result=operator(execute_ast(u->left, scope, 0), execute_ast(u->right, scope, 0), u->op);
            break;
        }
        case _prefix:
        {
            prefix* p=(prefix*)exp;
            object* null_object=new_null();
            result=operator(execute_ast(p->right, scope, 0), null_object, p->op);
            object_delete(null_object);
            break;
        }
        case _conditional:
        {
            conditional* c=(conditional*)exp;
            if(is_falsy(execute_ast(c->condition, scope, 0))){
                result=execute_ast((expression*)c->onfalse, scope, 0);
            } else{
                result=execute_ast((expression*)c->ontrue, scope, 0);
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
            scope->ref_count++;
            f->pointer=ast_function_call;
            result=(object*)f;
            break;
        }
        case _function_call:
        {
            function_call* c=(function_call*)exp;
            table* function_scope=new_table();
            function* f=(function*)path_get(scope, *c->function_path);
            if(f->enclosing_scope!=NULL){
                setup_scope((object*)function_scope, (object*)f->enclosing_scope);
            } else {
                setup_scope((object*)function_scope, (object*)scope);
            }
            if(vector_total(&c->arguments->lines)<vector_total(&f->argument_names)){
                ERROR(WRONG_ARGUMENT_TYPE, "Not enough arguments in function call.");
            }
            for (int i = 0; i < vector_total(&c->arguments->lines); i++){
                //char buf[16];
                //itoa(i, buf, 10);
                char* argument_name=vector_get(&f->argument_names, i);
                object* argument_value=execute_ast(vector_get(&c->arguments->lines, i), scope, 0);
                set((object*)function_scope, argument_name, argument_value);
            }
            result=(object*)call((object*)f, function_scope);
            break;
        }
        case _path:
        {
            path* p=(path*)exp;
            result=path_get(scope, *p);
            break;
        }
        default:
        {
            printf("uncatched expression type: %i\n", exp->type);
            result=(object*)new_null();
        }
    }
    return result;
}