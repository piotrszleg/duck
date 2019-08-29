#include "execute_ast.h"

void ast_execution_state_init(ASTExecutionState* state) {
    state->returning=false;
    vector_init(&state->used_objects, sizeof(Object), 8);
}

void ast_execution_state_deinit(ASTExecutionState* state) {
    vector_deinit(&state->used_objects);
}

Object path_get(Executor* E, Object scope, path p){
    Object current=scope;
    int lines_count=vector_count(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= pointers_vector_get(&p.lines, i);
        Object object_at_name;
        if(e->type==e_name){
            object_at_name=get(E, current, to_string(((name*)e)->value));
        } else {
            Object key=execute_ast(E, e, false);
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
            Object key=execute_ast(E, e, false);
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

void ast_source_pointer_init(Executor* E, ASTSourcePointer* source_pointer);

void ast_source_pointer_free(Executor* E, ASTSourcePointer* source_pointer){
    delete_expression(source_pointer->body);
    free(source_pointer);
}

ASTSourcePointer* ast_source_pointer_copy(Executor* E, ASTSourcePointer* source){
    ASTSourcePointer* copied=malloc(sizeof(ASTSourcePointer));
    copied->body=copy_expression(source->body);
    ast_source_pointer_init(E, copied);
    return copied;
}

void ast_source_pointer_init(Executor* E, ASTSourcePointer* source_pointer){
    managed_pointer_init(E, (ManagedPointer*)source_pointer, (ManagedPointerFreeFunction)ast_source_pointer_free);
    source_pointer->mp.copy=(ManagedPointerCopyFunction)ast_source_pointer_copy;
}

Object execute_ast(Executor* E, expression* exp, bool keep_scope){
    if(exp==NULL){
        return null_const;
    }
    E->line=exp->line_number;
    E->column=exp->column_number;
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
            return to_string(strdup(((string_literal*)exp)->value));
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
                            set_result=set(E, table, to_string(((name*)e)->value), execute_ast(E, a->right, false));
                        } else {
                            set_result=set(E, table, execute_ast(E, e, false), execute_ast(E, a->right, false));
                        }
                    }
                } else {
                    set_result=set(E, table, to_int(array_counter++), execute_ast(E, line, false));
                }
                destroy_unreferenced(E, &set_result);
            }
            RETURN_USED(table)
        }
        case e_block:
        {
            block* b=(block*)exp;
            Object previous_scope=E->scope;
            if(!keep_scope) {
                Object block_scope;
                table_init(E, &block_scope);
                inherit_scope(E, block_scope, E->scope);
                E->scope=block_scope;
            }
            Object result;
            for (int i = 0; i < vector_count(&b->lines); i++){
                Object line_result=execute_ast(E, pointers_vector_get(&b->lines, i), false);
                reference(&line_result);
                if(E->ast_execution_state.returning || i == vector_count(&b->lines)-1){
                    result=line_result;
                    USE(result)
                    break;
                } else {
                    dereference(E, &line_result);
                }
            }
            E->scope=previous_scope;
            if(!E->options.disable_garbage_collector && gc_should_run(E->object_system.gc)){
                executor_collect_garbage(E);
            }
            RETURN_USED(result)
        }
        case e_name:
        {
            return get(E, E->scope, to_string(((name*)exp)->value));
        }
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            Object result=execute_ast(E, a->right, false);
            USE(result)
            path_set(E, E->scope, *a->left, result);
            STOP_USING(result)
            return result;
        }
        case e_binary:
        {
            binary* u=(binary*)exp;
            Object left=execute_ast(E, u->left, false);
            USE(left)
            Object right=execute_ast(E, u->right, false);
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
            Object right=execute_ast(E, p->right, false);
            USE(right)
            Object result=operator(E, left, right, p->op);
            STOP_USING(right)
            return result;
        }
        case e_conditional:
        {
            conditional* c=(conditional*)exp;
            Object condition=execute_ast(E, c->condition, false);
            USE(condition)
            Object result;
            if(is_falsy(condition)){
                result=execute_ast(E, (expression*)c->onfalse, false);
            } else{
                result=execute_ast(E, (expression*)c->ontrue, false);
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
            f.fp->enclosing_scope=E->scope;
            reference(&E->scope);

            if(!E->options.disable_bytecode){
                f.fp->ftype=f_bytecode;
                BytecodeProgram* bytecode_program=ast_function_to_bytecode(d);
                bytecode_program->source_file_name=strdup(E->file);
                bytecode_program_init(E, bytecode_program);
                heap_object_reference((HeapObject*)bytecode_program);
                if(E->options.optimise_bytecode){
                    optimise_bytecode(E, bytecode_program, E->options.print_bytecode_optimisations);
                }
                if(E->options.print_bytecode){
                    print_bytecode_program(bytecode_program);
                }
                f.fp->source_pointer=(HeapObject*)bytecode_program;
            } else {
                f.fp->ftype=f_ast;
                ASTSourcePointer* source_pointer=malloc(sizeof(ASTSourcePointer));
                source_pointer->body=copy_expression(d->body);
                ast_source_pointer_init(E, source_pointer);
                heap_object_reference((HeapObject*)source_pointer);
                f.fp->source_pointer=(HeapObject*)source_pointer;
            }
            return f;
        }
        case e_function_call:
        {
            function_call* c=(function_call*)exp;
            
            TracebackPoint traceback_point={strdup("input"), exp->line_number};
            vector_push(&E->traceback, &traceback_point);

            Object f=execute_ast(E, c->called, false);
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
                    Object argument_value=execute_ast(E, pointers_vector_get(&c->arguments->lines, i), false);
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
            return path_get(E, E->scope, *(path*)exp);
        }
        case e_parentheses:
        {
            return execute_ast(E, ((parentheses*)exp)->value, false);
        }
        case e_function_return:
        {
            function_return* r=(function_return*)exp;
            Object result=execute_ast(E, r->value, false);
            E->ast_execution_state.returning=true;
            return result;
        }
        case e_question_mark:
        {
            question_mark* q=(question_mark*)exp;
            Object result=execute_ast(E, q->value, false);
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