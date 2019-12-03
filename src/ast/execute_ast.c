#include "execute_ast.h"

Object ast_set(Executor* E, Object scope, Expression* expression, Object value){
    switch(expression->type){
        case e_name:
        {
            Name* n=(Name*)expression;
            return set(E, scope, to_string(n->value), value);
        }
        case e_member_access:
        {
            MemberAccess* m=(MemberAccess*)expression;
            Object indexed=execute_ast(E, m->left, false);
            Object result=set(E, indexed, to_string(m->right->value), value);
            dereference(E, &indexed);
            return result;
        }
        case e_indexer:
        {
            Indexer* i=(Indexer*)expression;
            Object indexed=execute_ast(E, i->left, false);
            Object key=execute_ast(E, i->right, false);
            Object result=set(E, indexed, key, value);
            dereference(E, &key);
            dereference(E, &indexed);
            return result;
        }
        default:
            USING_STRING(stringify_expression(expression, 0),
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

#define USE(object) \
    vector_push(&E->stack, &object);
#define STOP_USING(object) \
    objects_vector_delete(E, &E->stack, object); \
    dereference(E, &object);
#define RETURN_USED(object) \
    objects_vector_delete(E, &E->stack, object); \
    return object;

Object execute_ast(Executor* E, Expression* expression, bool keep_scope){
    if(expression==NULL){
        return null_const;
    }
    E->line=expression->line_number;
    E->column=expression->column_number;

    if(E->options.debug){
        debugger_update(E, &E->debugger);
    }
    
    switch(expression->type){
        case e_expression:
        case e_macro:
        case e_macro_declaration:
        case e_empty:
        case e_null_literal:
            return null_const;
        case e_float_literal:
            return to_float(((FloatLiteral*)expression)->value);
        case e_int_literal:
            return to_int(((IntLiteral*)expression)->value);
        case e_string_literal:
            return to_string(strdup(((StringLiteral*)expression)->value));
        case e_table_literal:
        {
            Block* b=(Block*)expression;
            Object table;
            table_init(E, &table);
            USE(table)
            int array_counter=0;
            for (int i = 0; i < vector_count(&b->lines); i++){
                Expression* line=(Expression*)pointers_vector_get(&b->lines, i);
                Object set_result=null_const;
                if(line->type==e_assignment){
                    Assignment* a=(Assignment*)line;
                    switch(a->left->type){
                        case e_self_indexer: {
                            SelfIndexer* si=(SelfIndexer*)a->left;
                            set_result=set(E, table, execute_ast(E, si->right, false), 
                                          execute_ast(E, a->right, false));
                            break;
                        }
                        case e_name: {
                            Name* n=(Name*)a->left;
                            set_result=set(E, table, to_string(n->value), execute_ast(E, a->right, false));
                            break;
                        }
                        case e_self_member_access: {
                            SelfMemberAccess* sma=(SelfMemberAccess*)a->left;
                            set_result=set(E, table, to_string(sma->right->value), execute_ast(E, a->right, false));
                            break;
                        }
                        default: 
                            THROW_ERROR(AST_ERROR, "Incorrect expression inside of table literal.");
                    }
                } else {
                    set_result=set(E, table, to_int(array_counter++), execute_ast(E, line, false));
                }
                dereference(E, &set_result);
            }
            RETURN_USED(table)
        }
        case e_block:
        {
            Block* b=(Block*)expression;
            Object previous_scope=E->scope;
            if(!keep_scope) {
                Object block_scope;
                table_init(E, &block_scope);
                inherit_scope(E, block_scope, E->scope);
                USE(E->scope)
                E->scope=block_scope;
            }
            Object result;
            for (int i = 0; i < vector_count(&b->lines); i++){
                Object line_result=execute_ast(E, pointers_vector_get(&b->lines, i), false);
                if(E->returning || i == vector_count(&b->lines)-1){
                    result=line_result;
                    USE(result)
                    break;
                } else {
                    dereference(E, &line_result);
                }
            }
            if(!keep_scope) {
                vector_pop(&E->stack);
                dereference(E, &E->scope);
                E->scope=previous_scope;
            }
            if(!E->options.disable_garbage_collector && gc_should_run(E->object_system.gc)){
                executor_collect_garbage(E);
            }
            RETURN_USED(result)
        }
        case e_assignment:
        {
            Assignment* a=(Assignment*)expression;
            Object value=execute_ast(E, a->right, false);
            USE(value)
            Object result=ast_set(E, E->scope, a->left, value);
            USE(result)
            STOP_USING(value)
            RETURN_USED(result)
        }
        case e_binary:
        {
            Binary* u=(Binary*)expression;
            Object left=execute_ast(E, u->left, false);
            USE(left)
            Object right=execute_ast(E, u->right, false);
            USE(right)
            Object result=operator(E, left, right, u->op);
            // USE(result)
            STOP_USING(left)
            STOP_USING(right)
            RETURN_USED(result)
        }
        case e_prefix:
        {
            Prefix* p=(Prefix*)expression;
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
            Conditional* c=(Conditional*)expression;
            Object condition=execute_ast(E, c->condition, false);
            USE(condition)
            Object result;
            if(is_falsy(condition)){
                result=execute_ast(E, (Expression*)c->onfalse, false);
            } else{
                result=execute_ast(E, (Expression*)c->ontrue, false);
            }
            USE(result)
            STOP_USING(condition)
            RETURN_USED(result)
        }
        case e_function_declaration:
        {
            FunctionDeclaration* d=(FunctionDeclaration*)expression;
            int arguments_count=vector_count(&d->arguments);
            Object f;
            function_init(E, &f);
            f.fp->argument_names=malloc(sizeof(char*)*arguments_count);
            f.fp->arguments_count=arguments_count;
            f.fp->variadic=d->variadic;
            for (int i = 0; i < arguments_count; i++){
                f.fp->argument_names[i]=strdup(((Argument*)pointers_vector_get(&d->arguments, i))->name);
            }
            f.fp->enclosing_scope=E->scope;
            reference(&E->scope);
            USE(f)
            BytecodeProgram* bytecode_program;
            if(!E->options.disable_bytecode 
                && (bytecode_program=ast_function_to_bytecode(d))!=NULL){
                f.fp->ftype=f_bytecode;
                bytecode_program->source_file_name=strdup(E->file);
                if(E->options.optimise_bytecode){
                    optimise_bytecode(E, bytecode_program, E->options.print_bytecode_optimisations);
                }
                bytecode_program_init(E, bytecode_program);
                if(E->options.print_bytecode){
                    print_bytecode_program(bytecode_program);
                }
                if(E->options.compile_bytecode){
                    compile_bytecode_program(E, bytecode_program);
                }
                f.fp->source_pointer=(HeapObject*)bytecode_program;
            } else {
                f.fp->ftype=f_ast;
                ASTSourcePointer* source_pointer=malloc(sizeof(ASTSourcePointer));
                source_pointer->body=copy_expression(d->body);
                ast_source_pointer_init(E, source_pointer);
                f.fp->source_pointer=(HeapObject*)source_pointer;
            }
            RETURN_USED(f)
        }
        case e_function_call:
        {
            FunctionCall* c=(FunctionCall*)expression;
            
            TracebackPoint traceback_point={"input", expression->line_number};
            vector_push(&E->traceback, &traceback_point);

            Object f=execute_ast(E, c->called, false);
            USE(f)
            if(f.type==t_function && f.fp->ftype==f_special){
                switch(f.fp->special_index){
                    default:
                        STOP_USING(f)
                        RETURN_ERROR("CALL_ERROR", f, "Unknown special function of special_index %i.", f.fp->special_index)
                
                }
            } else {
                int arguments_count=vector_count(&c->arguments->lines);
                Object* arguments=malloc(arguments_count*sizeof(Object));
                for (int i = 0; i < arguments_count; i++){
                    arguments[i]=execute_ast(E, pointers_vector_get(&c->arguments->lines, i), false);
                    USE(arguments[i])
                }
                Object result=call(E, f, arguments, arguments_count);
                USE(result)
                E->returning=false;
                for (int i = 0; i < arguments_count; i++){
                    STOP_USING(arguments[i])
                }
                free(arguments);
                STOP_USING(f)
                RETURN_USED(result)
            }
        }
        case e_parentheses:
        {
            return execute_ast(E, ((Parentheses*)expression)->value, false);
        }
        case e_name:
        {
            Name* n=(Name*)expression;
            Object result=get(E, E->scope, to_string(n->value));
            RETURN_USED(result);
        }
        case e_member_access:
        {
            MemberAccess* m=(MemberAccess*)expression;
            Object indexed=execute_ast(E, m->left, false);
            USE(indexed)
            Object result=get(E, indexed, to_string(m->right->value));
            USE(result)
            STOP_USING(indexed)
            RETURN_USED(result)
        }
        case e_indexer:
        {
            Indexer* i=(Indexer*)expression;
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
            FunctionReturn* r=(FunctionReturn*)expression;
            Object result=execute_ast(E, r->value, false);
            USE(result)
            E->returning=true;
            RETURN_USED(result)
        }
        case e_return_if_error:
        {
            ReturnIfError* re=(ReturnIfError*)expression;
            Object result=execute_ast(E, re->value, false);
            USE(result)
            if(is_error(E, result)){
                E->returning=true;
            }
            RETURN_USED(result)
        }
        default:
        {
            RETURN_ERROR("AST_EXECUTION_ERROR", null_const, "Uncatched expression type: %i\n", expression->type)
        }
    }
}

#undef USE
#undef STOP_USING
#undef RETURN_USED