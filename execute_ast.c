#include "execute_ast.h"

Object path_get(Executor* E, Object scope, path p){
    Object current=scope;
    int lines_count=vector_count(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= pointers_vector_get(&p.lines, i);
        Object object_at_name;
        if(e->type==e_name){
            object_at_name=get(E, current, to_string(((name*)e)->value));
        } else {
            Object key=execute_ast(E, e, scope, 0);
            reference(&key);
            object_at_name=get(E, current, key);
            dereference(E, &key);
        }
        if(i==lines_count-1){
            return object_at_name;
        } else {
            current=object_at_name;
        }
    }
    return null_const;
} 

void path_set(Executor* E, Object scope, path p, Object value){
    Object current=scope;
    int lines_count=vector_count(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= pointers_vector_get(&p.lines, i);
        Object key;
        if(e->type==e_name){
            key=to_string(((name*)e)->value);
        } else {
            Object key=execute_ast(E, e, scope, 0);
            reference(&key);
        }
        if(i==lines_count-1){
            set(E, current, key, value);
        } else{
            Object object_at_name=get(E, current, key);
            current=object_at_name;
        }
        if(e->type!=e_name){// we don't borrow name expression's string value
            dereference(E, &key);
        }
    }
}

void ast_source_pointer_destructor(Executor* E, ASTSourcePointer* sp){
    delete_expression(sp->body);
    free(sp);
}

Object execute_ast(Executor* E, expression* exp, Object scope, int keep_scope){
    if(exp==NULL){
        return null_const;
    }
    E->line=exp->line_number;
    E->column=exp->column_number;
    switch(exp->type){
        case e_expression:
        case e_macro:
        case e_macro_declaration:
        case e_empty:
            return null_const;
        case e_float_literal:
            return to_float(((float_literal*)exp)->value);
        case e_int_literal:
            return to_int(((int_literal*)exp)->value);
        case e_string_literal:
            return to_string(((string_literal*)exp)->value);
        case e_table_literal:
        {
            block* b=(block*)exp;
            Object table_scope;
            table_init(E, &table_scope);
            reference(&table_scope);
            int array_counter=0;
            for (int i = 0; i < vector_count(&b->lines); i++){
                expression* line=(expression*)pointers_vector_get(&b->lines, i);
                Object set_result;
                if(line->type==e_assignment){
                    assignment* a=(assignment*)line;
                    set_result=set(E, table_scope, to_string(table_literal_extract_key(a)), execute_ast(E, a->right, table_scope, 0));
                } else {
                    set_result=set(E, table_scope, to_int(array_counter++), execute_ast(E, line, table_scope, 0));
                }
                destroy_unreferenced(E, &set_result);
            }
            return table_scope;
        }
        case e_block:
        {
            block* b=(block*)exp;
            Object block_scope;
            if(keep_scope){
                block_scope=scope;
            } else {
                table_init(E, &block_scope);
                inherit_scope(E, block_scope, scope);
            }
            Object result;
            for (int i = 0; i < vector_count(&b->lines); i++){
                Object line_result=execute_ast(E, pointers_vector_get(&b->lines, i), block_scope, 0);
                reference(&line_result);
                if(E->ast_returning || i == vector_count(&b->lines)-1){
                    result=line_result;
                    break;
                } else {
                    dereference(E, &line_result);
                }
            }
            if(!keep_scope){
                dereference(E, &block_scope);
            }
            return result;
        }
        case e_name:
        {
            return get(E, scope, to_string(((name*)exp)->value));
        }
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            Object result=execute_ast(E, a->right, scope, 0);
            path_set(E, scope, *a->left, result);
            return result;
        }
        case e_binary:
        {
            binary* u=(binary*)exp;
            Object left=execute_ast(E, u->left, scope, 0);
            Object right=execute_ast(E, u->right, scope, 0);
            Object result=operator(E, left, right, u->op);
            dereference(E, &left);
            dereference(E, &right);
            return result;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            Object left=null_const;
            Object right=execute_ast(E, p->right, scope, 0);
            Object result=operator(E, left, right, p->op);
            dereference(E, &left);
            return result;
        }
        case e_conditional:
        {
            conditional* c=(conditional*)exp;
            if(is_falsy(execute_ast(E, c->condition, scope, 0))){
                return execute_ast(E, (expression*)c->onfalse, scope, 0);
            } else{
                return execute_ast(E, (expression*)c->ontrue, scope, 0);
            }
        }
        case e_function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;
            int arguments_count=vector_count(&d->arguments);

            Object f;
            function_init(E, &f);
            f.fp->argument_names=malloc(sizeof(char*)*arguments_count);
            f.fp->arguments_count=arguments_count;
            f.fp->variadic=d->variadic;

            for (int i = 0; i < arguments_count; i++){
                f.fp->argument_names[i]=strdup(((argument*)pointers_vector_get(&d->arguments, i))->name);
            }
            f.fp->ftype=f_ast;
            ASTSourcePointer* sp=malloc(sizeof(ASTSourcePointer));
            sp->body=copy_expression(d->body);
            gc_pointer_init(E, (gc_Pointer*)sp, (gc_PointerDestructorFunction)ast_source_pointer_destructor);
            f.fp->source_pointer=(gc_Object*)sp;
            f.fp->enclosing_scope=scope;
            reference(&scope);
            return f;
        }
        case e_function_call:
        {
            function_call* c=(function_call*)exp;
            
            Object f=execute_ast(E, c->called, scope, 0);
            int arguments_count=vector_count(&c->arguments->lines);
            Object* arguments=malloc(arguments_count*sizeof(Object));
            for (int i = 0; i < vector_count(&c->arguments->lines); i++){
                Object argument_value=execute_ast(E, pointers_vector_get(&c->arguments->lines, i), scope, 0);
                arguments[i]=argument_value;
            }
            Object result=call(E, f, arguments, arguments_count);
            E->ast_returning=false;
            free(arguments);
            return result;
        }
        case e_path:
        {
            return path_get(E, scope, *(path*)exp);
        }
        case e_parentheses:
        {
            return execute_ast(E, ((parentheses*)exp)->value, scope, 0);
        }
        case e_function_return:
        {
            function_return* r=(function_return*)exp;
            Object result=execute_ast(E, r->value, scope, 0);
            E->ast_returning=true;
            return result;
        }
        default:
        {
            printf("uncatched expression type: %i\n", exp->type);
            return null_const;
        }
    }
}