#include "ast_executor.h"

// creates string variable str, executes body and frees the string afterwards
#define USING_STRING(string_expression, body) { char* str=string_expression; body; free(str); }

void copy_table(table_* source, table_* destination){
    const char *key;
    map_iter_t iter = map_iter(&source->fields);
    while ((key = map_next(&source->fields, &iter))) {
        map_set(&destination->fields, key, (*map_get(&source->fields, key)));
    }
}

object path_get(ast_executor_state* state, object scope, path p){
    object current=scope;
    int lines_count=vector_total(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= vector_get(&p.lines, i);
        char* evaluated_to_string;
        if(e->type==_name){
            evaluated_to_string=((name*)e)->value;
        } else {
            USING_STRING(stringify(execute_ast(state, e, scope, 0)), 
                evaluated_to_string=str);
        }
        object object_at_name=get(current, evaluated_to_string);
        if(i==lines_count-1){
            return object_at_name;
        } else {
            current=object_at_name;
        }
    }
    return null_const;
} 

void path_set(ast_executor_state* state, object scope, path p, object value){
    object current=scope;
    int lines_count=vector_total(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= vector_get(&p.lines, i);
        char* evaluated_to_string;
        if(e->type==_name){
            evaluated_to_string=((name*)e)->value;
        } else {
            USING_STRING(stringify(execute_ast(state, e, scope, 0)), 
                evaluated_to_string=str);
        }
        if(i==lines_count-1){
            set(current, evaluated_to_string, value);
        } else{
            object object_at_name=get(current, evaluated_to_string);
            current=object_at_name;
        }
    }
}

object execute_ast(ast_executor_state* state, expression* exp, object scope, int keep_scope){
    if(exp==NULL){
        ERROR(INCORRECT_OBJECT_POINTER, "AST expression pointer is null.");
    }
    state->line_number=exp->line_number;
    state->column_number=exp->column_number;
    switch(exp->type){
        case _empty:
            return null_const;
        case _literal:
        {
            literal* l=(literal*)exp;
            object result;
            switch(l->ltype){
                case _int:
                    number_init(&result);
                    result.value=l->ival;
                    break;
                case _float:
                    number_init(&result);
                    result.value=l->fval;
                    break;
                case _string:
                    string_init(&result);
                    result.text=strdup(l->sval);
                    break;
            }
            return result;
        }
        case _table_literal:
        {
            block* b=(block*)exp;
            object table_scope;
            table_init(&table_scope);
            reference(&table_scope);
            for (int i = 0; i < vector_total(&b->lines); i++){
                expression* line=(expression*)vector_get(&b->lines, i);
                object line_result=execute_ast(state, line, table_scope, 0);
                if(line->type!=_assignment){
                    char buf[16];
                    sprintf(buf,"%d",i);
                    set(table_scope, buf, line_result);
                }
                object_deinit(&line_result);
            }
            return table_scope;
        }
        case _block:
        {
            block* b=(block*)exp;
            object block_scope;
            if(keep_scope){
                block_scope=scope;
            } else {
                table_init(&block_scope);
                inherit_scope(block_scope, scope);
            }
            object result;
            for (int i = 0; i < vector_total(&b->lines); i++){
                object line_result=execute_ast(state, vector_get(&b->lines, i), block_scope, 0);
                if(state->returning || i == vector_total(&b->lines)-1){
                    reference(&line_result);
                    result=line_result;
                    break;
                } else {
                    object_deinit(&line_result);
                }
            }
            if(!keep_scope){
                object_deinit(&block_scope);
            }
            return result;
        }
        case _name:
        {
            return get(scope, ((name*)exp)->value);
        }
        case _assignment:
        {
            assignment* a=(assignment*)exp;
            object result=execute_ast(state, a->right, scope, 0);
            path_set(state, scope, *a->left, result);
            return result;
        }
        case _unary:
        {
            unary* u=(unary*)exp;
            object left=execute_ast(state, u->left, scope, 0);
            object right=execute_ast(state, u->right, scope, 0);
            object result=operator(left, right, u->op);
            object_deinit(&left);
            object_deinit(&right);
            return result;
        }
        case _prefix:
        {
            prefix* p=(prefix*)exp;
            object left=execute_ast(state, p->right, scope, 0);
            object right=null_const;
            object result=operator(left, right, p->op);
            object_deinit(&left);
            return result;
        }
        case _conditional:
        {
            conditional* c=(conditional*)exp;
            if(is_falsy(execute_ast(state, c->condition, scope, 0))){
                return execute_ast(state, (expression*)c->onfalse, scope, 0);
            } else{
                return execute_ast(state, (expression*)c->ontrue, scope, 0);
            }
        }
        case _function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;
            object f;
            function_init(&f);
            vector_init(&f.fp->argument_names);
            for (int i = 0; i < vector_total(d->arguments); i++){
                vector_add(&f.fp->argument_names, ((name*)vector_get(d->arguments, i))->value);
            }
            f.fp->ftype=f_ast;
            f.fp->ast_pointer=(void*)d->body;
            f.fp->enviroment=(void*)state;
            f.fp->enclosing_scope=malloc(sizeof(scope));
            memcpy(f.fp->enclosing_scope, &scope, sizeof(scope));
            reference(f.fp->enclosing_scope);
            return f;
        }
        case _function_call:
        {
            function_call* c=(function_call*)exp;
            
            object f=path_get(state, scope, *c->function_path);
            int arguments_count=vector_total(&c->arguments->lines);
            object* arguments=malloc(arguments_count*sizeof(object));
            for (int i = 0; i < vector_total(&c->arguments->lines); i++){
                object argument_value=execute_ast(state, vector_get(&c->arguments->lines, i), scope, 0);
                arguments[i]=argument_value;
            }
            object result=call(f, arguments, arguments_count);
            state->returning=false;
            free(arguments);
            return result;
        }
        case _path:
        {
            return path_get(state, scope, *(path*)exp);
        }
        case _function_return:
        {
            function_return* r=(function_return*)exp;
            object result=execute_ast(state, r->value, scope, 0);
            state->returning=true;
            return result;
        }
        default:
        {
            printf("uncatched expression type: %i\n", exp->type);
            return null_const;
        }
    }
}