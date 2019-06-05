#include "execute_ast.h"

object path_get(executor* Ex, object scope, path p){
    object current=scope;
    int lines_count=vector_total(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= vector_get(&p.lines, i);
        object object_at_name;
        if(e->type==e_name){
            object_at_name=get(Ex, current, to_string(((name*)e)->value));
        } else {
            object key=execute_ast(Ex, e, scope, 0);
            reference(&key);
            object_at_name=get(Ex, current, key);
            dereference(Ex, &key);
        }
        if(i==lines_count-1){
            return object_at_name;
        } else {
            current=object_at_name;
        }
    }
    return null_const;
} 

void path_set(executor* Ex, object scope, path p, object value){
    object current=scope;
    int lines_count=vector_total(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= vector_get(&p.lines, i);
        object key;
        if(e->type==e_name){
            key=to_string(((name*)e)->value);
        } else {
            object key=execute_ast(Ex, e, scope, 0);
            reference(&key);
        }
        if(i==lines_count-1){
            set(Ex, current, key, value);
        } else{
            object object_at_name=get(Ex, current, key);
            current=object_at_name;
        }
        if(e->type!=e_name){// we don't borrow name expression's string value
            dereference(Ex, &key);
        }
    }
}

object execute_ast(executor* Ex, expression* exp, object scope, int keep_scope){
    if(exp==NULL){
        THROW_ERROR(INCORRECT_OBJECT_POINTER, "AST expression pointer is null.");
    }
    Ex->line=exp->line_number;
    Ex->column=exp->column_number;
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
            int array_counter=0;
            for (int i = 0; i < vector_total(&b->lines); i++){
                expression* line=(expression*)vector_get(&b->lines, i);
                object set_result;
                if(line->type==e_assignment){
                    assignment* a=(assignment*)line;
                    set_result=set(Ex, table_scope, to_string(table_literal_extract_key(a)), execute_ast(Ex, a->right, table_scope, 0));
                } else {
                    set_result=set(Ex, table_scope, to_number(array_counter++), execute_ast(Ex, line, table_scope, 0));
                }
                destroy_unreferenced(Ex, &set_result);
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
                inherit_scope(Ex, block_scope, scope);
            }
            object result;
            for (int i = 0; i < vector_total(&b->lines); i++){
                object line_result=execute_ast(Ex, vector_get(&b->lines, i), block_scope, 0);
                reference(&line_result);
                if(Ex->ast_returning || i == vector_total(&b->lines)-1){
                    result=line_result;
                    break;
                } else {
                    dereference(Ex, &line_result);
                }
            }
            if(!keep_scope){
                dereference(Ex, &block_scope);
            }
            return result;
        }
        case e_name:
        {
            return get(Ex, scope, to_string(((name*)exp)->value));
        }
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            object result=execute_ast(Ex, a->right, scope, 0);
            path_set(Ex, scope, *a->left, result);
            return result;
        }
        case e_binary:
        {
            binary* u=(binary*)exp;
            object left=execute_ast(Ex, u->left, scope, 0);
            object right=execute_ast(Ex, u->right, scope, 0);
            object result=operator(Ex, left, right, u->op);
            dereference(Ex, &left);
            dereference(Ex, &right);
            return result;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            object left=execute_ast(Ex, p->right, scope, 0);
            object right=null_const;
            object result=operator(Ex, left, right, p->op);
            dereference(Ex, &left);
            return result;
        }
        case e_conditional:
        {
            conditional* c=(conditional*)exp;
            if(is_falsy(execute_ast(Ex, c->condition, scope, 0))){
                return execute_ast(Ex, (expression*)c->onfalse, scope, 0);
            } else{
                return execute_ast(Ex, (expression*)c->ontrue, scope, 0);
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
            f.fp->variadic=d->variadic;

            for (int i = 0; i < arguments_count; i++){
                f.fp->argument_names[i]=strdup(((name*)vector_get(d->arguments, i))->value);
            }
            f.fp->ftype=f_ast;
            f.fp->source_pointer=(void*)copy_expression(d->body);
            f.fp->environment=NULL;
            f.fp->enclosing_scope=scope;
            reference(&scope);
            return f;
        }
        case e_function_call:
        {
            function_call* c=(function_call*)exp;
            
            object f=execute_ast(Ex, c->called, scope, 0);
            int arguments_count=vector_total(&c->arguments->lines);
            object* arguments=malloc(arguments_count*sizeof(object));
            for (int i = 0; i < vector_total(&c->arguments->lines); i++){
                object argument_value=execute_ast(Ex, vector_get(&c->arguments->lines, i), scope, 0);
                arguments[i]=argument_value;
            }
            object result=call(Ex, f, arguments, arguments_count);
            Ex->ast_returning=false;
            free(arguments);
            return result;
        }
        case e_path:
        {
            return path_get(Ex, scope, *(path*)exp);
        }
        case e_function_return:
        {
            function_return* r=(function_return*)exp;
            object result=execute_ast(Ex, r->value, scope, 0);
            Ex->ast_returning=true;
            return result;
        }
        default:
        {
            printf("uncatched expression type: %i\n", exp->type);
            return null_const;
        }
    }
}