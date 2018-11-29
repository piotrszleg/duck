#include "ast_executor.h"

int current_line;

// creates string variable str, executes body and frees the string afterwards
#define USING_STRING(string_expression, body) { char* str=string_expression; body; free(str); }

void copy_table(table* source, table* destination){
    const char *key;
    map_iter_t iter = map_iter(&source->fields);
    while ((key = map_next(&source->fields, &iter))) {
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
            USING_STRING(stringify(execute_ast(e, scope, 0)), 
                evaluated_to_string=str);
        }
        object* object_at_name=get((object*)current, evaluated_to_string);
        if(i==lines_count-1){
            return object_at_name;
        } else if(object_at_name->type==t_table) {
            current=(table*)object_at_name;
        } else {
            USING_STRING(stringify(object_at_name), 
                ERROR(INCORRECT_OBJECT_POINTER, "%s is not a table.", str));
            return (object*)new_null();
        }
    }
    return (object*)new_null();
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
            USING_STRING(stringify(execute_ast(e, scope, 0)), 
                evaluated_to_string=str);
        }
        if(i==lines_count-1){
            set((object*)current, evaluated_to_string, value);
        } else{
            object* object_at_name=get((object*)current, evaluated_to_string);
            if(object_at_name->type==t_table) {
                current=(table*)object_at_name;
            } else {
                USING_STRING(stringify(object_at_name), 
                    ERROR(INCORRECT_OBJECT_POINTER, "%s is not a table.", str));
            }
        }
    }
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
                    ((string*)result)->value=strdup(l->sval);
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
                object* line_result=execute_ast(line, new_scope, 0);
                if(line->type!=_assignment){
                    char buf[16];
                    sprintf(buf,"%d",i);
                    set((object*)new_scope, buf, line_result);
                }
                delete_unreferenced(line_result);
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
                setup_scope((object*)block_scope, (object*)scope);
            }
            for (int i = 0; i < vector_total(&b->lines); i++){
                object* line_result=execute_ast(vector_get(&b->lines, i), block_scope, 0);
                if(i == vector_total(&b->lines)-1){
                    result=line_result;// last line is the result of evaluation of the entire block
                    result->ref_count++;
                } else {
                    delete_unreferenced(line_result);
                }
            }
            if(!keep_scope){
                delete_unreferenced((object*)block_scope);
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
            object* left=execute_ast(u->left, scope, 0);
            object* right=execute_ast(u->right, scope, 0);
            result=operator(left, right, u->op);
            delete_unreferenced(left);
            delete_unreferenced(right);
            break;
        }
        case _prefix:
        {
            prefix* p=(prefix*)exp;
            object* left=execute_ast(p->right, scope, 0);
            object* right=(object*)new_null();
            result=operator(left, right, p->op);
            delete_unreferenced(left);
            delete_unreferenced(right);
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
            vector_init(&f->argument_names);
            for (int i = 0; i < vector_total(d->arguments); i++){
                vector_add(&f->argument_names, ((name*)vector_get(d->arguments, i))->value);
            }
            f->f_type=f_ast;
            f->ast_pointer=(void*)d->body;
            f->enclosing_scope=scope;
            scope->ref_count++;
            result=(object*)f;
            break;
        }
        case _function_call:
        {
            function_call* c=(function_call*)exp;
            
            function* f=(function*)path_get(scope, *c->function_path);
            switch(f->f_type){
                case f_native:
                {
                    vector arguments;
                    vector_init(&arguments);
                    for (int i = 0; i < vector_total(&c->arguments->lines); i++){
                        object* argument_value=execute_ast(vector_get(&c->arguments->lines, i), scope, 0);
                        vector_add(&arguments, argument_value);
                    }
                    result=f->pointer(arguments);
                    vector_free(&arguments);
                }
                break;
                case f_ast:
                {
                    table* function_scope=new_table();
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
                    result=execute_ast(f->ast_pointer, function_scope, 1);
                    result->ref_count++;
                    delete_unreferenced((object*)function_scope);
                    break;
                }
                case f_bytecode:
                    ERROR(NOT_IMPLEMENTED, "Can't call ast function from bytecode\n");
                    break;
            }
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