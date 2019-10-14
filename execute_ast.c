#include "execute_ast.h"

void ast_execution_state_init(ASTExecutionState* state) {
    state->returning=false;
    vector_init(&state->used_objects, sizeof(Object), 8);
}

void ast_execution_state_deinit(ASTExecutionState* state) {
    vector_deinit(&state->used_objects);
}

Object ast_set(Executor* E, Object scope, expression* exp, Object value){
    switch(exp->type){
        case e_name:
        {
            name* n=(name*)exp;
            return set(E, scope, to_string(n->value), value);
        }
        case e_member_access:
        {
            member_access* m=(member_access*)exp;
            Object indexed=execute_ast(E, m->left, false);
            return set(E, indexed, to_string(m->right->value), value);
        }
        case e_indexer:
        {
            indexer* i=(indexer*)exp;
            Object indexed=execute_ast(E, i->left, false);
            Object key=execute_ast(E, i->right, false);
            return set(E, indexed, key, value);
        }
        default:
            USING_STRING(stringify_expression(exp, 0),
                THROW_ERROR(AST_ERROR, "Incorrect expression on left hand of assignment: \n%s", str);)
            return null_const;
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
                    switch(a->left->type){
                        case e_self_indexer: {
                            self_indexer* si=(self_indexer*)a->left;
                            set(E, table, execute_ast(E, si->right, false), 
                                          execute_ast(E, a->right, false));
                        }
                        case e_name: {
                            name* n=(name*)a->left;
                            set(E, table, to_string(n->value), execute_ast(E, a->right, false));
                        }
                        default: 
                            THROW_ERROR(AST_ERROR, "Incorrect expression inside of table literal.");
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
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            Object value=execute_ast(E, a->right, false);
            USE(value)
            Object result=ast_set(E, E->scope, a->left, value);
            STOP_USING(value)
            RETURN_USED(result)
        }
        case e_binary:
        {
            binary* u=(binary*)exp;
            Object left=execute_ast(E, u->left, false);
            USE(left)
            Object right=execute_ast(E, u->right, false);
            USE(right)
            Object result=operator(E, left, right, u->op);
            USE(result)
            STOP_USING(left)
            STOP_USING(right)
            RETURN_USED(result)
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            Object left=null_const;
            Object right=execute_ast(E, p->right, false);
            USE(right)
            Object result=operator(E, left, right, p->op);
            USE(result)
            STOP_USING(right)
            RETURN_USED(result)
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
            USE(result)
            STOP_USING(condition)
            RETURN_USED(result)
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
            USE(f)

            if(!E->options.disable_bytecode){
                f.fp->ftype=f_bytecode;
                BytecodeProgram* bytecode_program=ast_function_to_bytecode(d);
                bytecode_program->source_file_name=strdup(E->file);
                heap_object_reference((HeapObject*)bytecode_program);
                if(E->options.optimise_bytecode){
                    optimise_bytecode(E, bytecode_program, E->options.print_bytecode_optimisations);
                }
                bytecode_program_init(E, bytecode_program);
                if(E->options.print_bytecode){
                    print_bytecode_program(bytecode_program);
                }
                if(E->options.compile_bytecode_immediately){
                    compile_bytecode_program(E, bytecode_program);
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
            RETURN_USED(f)
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
                USE(result)
                E->ast_execution_state.returning=false;
                free(arguments);
                STOP_USING(f)
                RETURN_USED(result)
            }
        }
        case e_parentheses:
        {
            return execute_ast(E, ((parentheses*)exp)->value, false);
        }
        case e_name:
        {
            name* n=(name*)exp;
            Object result=get(E, E->scope, to_string(n->value));
            RETURN_USED(result);
        }
        case e_member_access:
        {
            member_access* m=(member_access*)exp;
            Object indexed=execute_ast(E, m->left, false);
            USE(indexed)
            Object result=get(E, indexed, to_string(m->right->value));
            USE(result)
            STOP_USING(indexed)
            RETURN_USED(result)
        }
        case e_indexer:
        {
            indexer* i=(indexer*)exp;
            Object indexed=execute_ast(E, i->left, false);
            USE(indexed)
            Object key=execute_ast(E, i->right, false);
            USE(key)
            Object result=get(E, indexed, key);
            USE(result)
            STOP_USING(indexed)
            STOP_USING(key)
            RETURN_USED(result)
        }
        case e_function_return:
        {
            function_return* r=(function_return*)exp;
            Object result=execute_ast(E, r->value, false);
            USE(result)
            E->ast_execution_state.returning=true;
            RETURN_USED(result)
        }
        case e_return_if_error:
        {
            return_if_error* re=(return_if_error*)exp;
            Object result=execute_ast(E, re->value, false);
            USE(result)
            if(is_error(E, result)){
                E->ast_execution_state.returning=true;
            }
            RETURN_USED(result)
        }
        default:
        {
            RETURN_ERROR("AST_EXECUTION_ERROR", null_const, "Uncatched expression type: %i\n", exp->type)
        }
    }
}