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

void ast_source_pointer_free(Executor* E, ASTSourcePointer* sp){
    delete_expression(sp->body);
    free(sp);
}

Object execute_ast(Executor* E, expression* exp, Object scope, int keep_scope){
    if(exp==NULL){
        return null_const;
    }
    E->line=exp->line_number;
    E->column=exp->column_number;
    E->scope=scope;
    #define USE(object) \
        vector_push(&E->ast_execution_state.used_objects, &object); \
        reference(&object);
    #define STOP_USING(object) \
        vector_delete_item(&E->ast_execution_state.used_objects, &object); \
        dereference(E, &object);
    #define RETURN_USED(object) \
        vector_delete_item(&E->ast_execution_state.used_objects, &object); \
        return object;
    
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
            Object table;
            table_init(E, &table);
            USE(table)
            reference(&table);
            int array_counter=0;
            for (int i = 0; i < vector_count(&b->lines); i++){
                expression* line=(expression*)pointers_vector_get(&b->lines, i);
                Object set_result;
                if(line->type==e_assignment){
                    assignment* a=(assignment*)line;
                    if(vector_count(&a->left->lines)!=1){
                        THROW_ERROR(WRONG_ARGUMENT_TYPE, "Number of lines in table literal key should be one.\n");
                    } else {
                        expression* e=pointers_vector_get(&a->left->lines, 0);
                        if(e->type==e_name) {
                            set_result=set(E, table, to_string(((name*)e)->value), execute_ast(E, a->right, scope, 0));
                        } else {
                            set_result=set(E, table, execute_ast(E, e, scope, 0), execute_ast(E, a->right, scope, 0));
                        }
                    }
                } else {
                    set_result=set(E, table, to_int(array_counter++), execute_ast(E, line, scope, 0));
                }
                destroy_unreferenced(E, &set_result);
            }
            RETURN_USED(table)
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
            USE(block_scope)
            Object result;
            for (int i = 0; i < vector_count(&b->lines); i++){
                Object line_result=execute_ast(E, pointers_vector_get(&b->lines, i), block_scope, 0);
                reference(&line_result);
                if(E->ast_execution_state.returning || i == vector_count(&b->lines)-1){
                    result=line_result;
                    USE(result)
                    break;
                } else {
                    dereference(E, &line_result);
                }
            }
            STOP_USING(block_scope)
            if(gc_should_run(E->gc)){
                executor_collect_garbage(E);
            }
            RETURN_USED(result)
        }
        case e_name:
        {
            return get(E, scope, to_string(((name*)exp)->value));
        }
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            Object result=execute_ast(E, a->right, scope, 0);
            USE(result)
            path_set(E, scope, *a->left, result);
            STOP_USING(result)
            return result;
        }
        case e_binary:
        {
            binary* u=(binary*)exp;
            Object left=execute_ast(E, u->left, scope, 0);
            USE(left)
            Object right=execute_ast(E, u->right, scope, 0);
            USE(right)
            Object result=operator(E, left, right, u->op);
            STOP_USING(left);
            STOP_USING(right);
            return result;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            Object left=null_const;
            Object right=execute_ast(E, p->right, scope, 0);
            USE(right)
            Object result=operator(E, left, right, p->op);
            STOP_USING(right)
            return result;
        }
        case e_conditional:
        {
            conditional* c=(conditional*)exp;
            Object condition=execute_ast(E, c->condition, scope, 0);
            USE(condition)
            Object result;
            if(is_falsy(condition)){
                result=execute_ast(E, (expression*)c->onfalse, scope, 0);
            } else{
                result=execute_ast(E, (expression*)c->ontrue, scope, 0);
            }
            STOP_USING(condition)
            return result;
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
            managed_pointer_init(E, (ManagedPointer*)sp, (ManagedPointerFreeFunction)ast_source_pointer_free);
            f.fp->source_pointer=(HeapObject*)sp;
            heap_object_reference((HeapObject*)sp);
            f.fp->enclosing_scope=scope;
            reference(&scope);
            return f;
        }
        case e_function_call:
        {
            function_call* c=(function_call*)exp;
            
            TracebackPoint traceback_point={strdup("input"), exp->line_number};
            vector_push(&E->traceback, &traceback_point);

            Object f=execute_ast(E, c->called, scope, 0);
            USE(f)
            if(f.type==t_function && f.fp->ftype==f_special){
                switch(f.fp->special_index){
                    case 2://  collect_garbage
                    {
                        executor_collect_garbage(E);
                        STOP_USING(f)
                        return null_const;
                    }
                    default:
                        STOP_USING(f)
                        RETURN_ERROR("CALL_ERROR", f, "Unknown special function of special_index %i.", f.fp->special_index)
                }
            } else {
                int arguments_count=vector_count(&c->arguments->lines);
                Object* arguments=malloc(arguments_count*sizeof(Object));
                for (int i = 0; i < vector_count(&c->arguments->lines); i++){
                    Object argument_value=execute_ast(E, pointers_vector_get(&c->arguments->lines, i), scope, 0);
                    arguments[i]=argument_value;
                }
                Object result=call(E, f, arguments, arguments_count);
                E->ast_execution_state.returning=false;
                free(arguments);
                STOP_USING(f)
                return result;
            }
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
            E->ast_execution_state.returning=true;
            return result;
        }
        case e_question_mark:
        {
            question_mark* q=(question_mark*)exp;
            Object result=execute_ast(E, q->value, scope, 0);
            USE(result)
            if(is_error(E, result)){
                E->ast_execution_state.returning=true;
            }
            RETURN_USED(result)
            return result;
        }
        default:
        {
            RETURN_ERROR("AST_EXECUTION_ERROR", null_const, "uncatched expression type: %i\n", exp->type)
        }
    }
}