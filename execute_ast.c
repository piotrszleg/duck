#include "execute_ast.h"

void copy_table(table* source, table* destination){
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
        if(e->type==e_name){
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
        if(e->type==e_name){
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
    exec_state.line=exp->line_number;
    exec_state.column=exp->column_number;
    switch(exp->type){
        case e_empty:
            return null_const;
        case e_literal:
        {
            literal* l=(literal*)exp;
            object result;
            switch(l->ltype){
                case l_int:
                    number_init(&result);
                    result.value=l->ival;
                    break;
                case l_float:
                    number_init(&result);
                    result.value=l->fval;
                    break;
                case l_string:
                    string_init(&result);
                    result.text=strdup(l->sval);
                    break;
            }
            return result;
        }
        case e_table_literal:
        {
            block* b=(block*)exp;
            object table_scope;
            table_init(&table_scope);
            reference(&table_scope);
            for (int i = 0; i < vector_total(&b->lines); i++){
                expression* line=(expression*)vector_get(&b->lines, i);
                object line_result=execute_ast(state, line, table_scope, 0);
                if(line->type!=e_assignment){
                    char buf[16];
                    sprintf(buf,"%d",i);
                    set(table_scope, buf, line_result);
                }
                dereference(&line_result);
            }
            return table_scope;
        }
        case e_block:
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
                reference(&line_result);
                if(state->returning || i == vector_total(&b->lines)-1){
                    result=line_result;
                    break;
                } else {
                    dereference(&line_result);
                }
            }
            if(!keep_scope){
                dereference(&block_scope);
            }
            return result;
        }
        case e_name:
        {
            return get(scope, ((name*)exp)->value);
        }
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            object result=execute_ast(state, a->right, scope, 0);
            path_set(state, scope, *a->left, result);
            return result;
        }
        case e_unary:
        {
            unary* u=(unary*)exp;
            object left=execute_ast(state, u->left, scope, 0);
            object right=execute_ast(state, u->right, scope, 0);
            object result=operator(left, right, u->op);
            dereference(&left);
            dereference(&right);
            return result;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            object left=execute_ast(state, p->right, scope, 0);
            object right=null_const;
            object result=operator(left, right, p->op);
            dereference(&left);
            return result;
        }
        case e_conditional:
        {
            conditional* c=(conditional*)exp;
            if(is_falsy(execute_ast(state, c->condition, scope, 0))){
                return execute_ast(state, (expression*)c->onfalse, scope, 0);
            } else{
                return execute_ast(state, (expression*)c->ontrue, scope, 0);
            }
        }
        case e_function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;
            int arguments_count=vector_total(d->arguments);

            object f;
            function_init(&f);
            f.fp->argument_names=malloc(sizeof(char*)*arguments_count);
            f.fp->arguments_count=arguments_count;

            for (int i = 0; i < arguments_count; i++){
                f.fp->argument_names[i]=strdup(((name*)vector_get(d->arguments, i))->value);
            }
            f.fp->ftype=f_ast;
            f.fp->source_pointer=(void*)copy_expression(d->body);
            f.fp->enviroment=(void*)state;
            f.fp->enclosing_scope=scope;
            reference(&scope);
            return f;
        }
        case e_function_call:
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
        case e_path:
        {
            return path_get(state, scope, *(path*)exp);
        }
        case e_function_return:
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