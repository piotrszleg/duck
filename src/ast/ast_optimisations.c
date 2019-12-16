#include "ast_optimisations.h"

#define LOG_CHANGE(message, before, after) \
    if(E->options.print_ast_optimisations){ \
        char* before_string=stringify_expression(before, 0); \
        char* after_string=stringify_expression(after, 0); \
        printf("\n%s\nreplacing:%s\nwith:%s\n", message, before_string, after_string); \
        free(before_string); \
        free(after_string); \
    }

bool expression_is_literal(Expression* expression){
    switch(expression->type){
        case e_empty:
        case e_null_literal:
        case e_int_literal:
        case e_float_literal:
        case e_string_literal:
        case e_function_declaration:
        case e_table_literal:
            return true;
        default:
            return false;
    }
}

ObjectType expression_object_type(Expression* expression){
    switch(expression->type){
        case e_empty:
        case e_null_literal: 
            return t_null;
        case e_int_literal: return t_int;
        case e_float_literal: return t_float;
        case e_string_literal: return t_string;
        case e_table_literal: return t_table;
        case e_function_declaration: return t_function;
        default: return t_any;
    }
}

bool expression_is_constant(Expression* expression){
    if(expression_is_literal(expression)){
        return true;
    }
    switch(expression->type){
        case e_binary:
        {
            Binary* b=(Binary*)expression;
            return operator_predict_result(expression_object_type(b->left), expression_object_type(b->right), b->op)!=t_any;
        }
        case e_prefix:
        {
            Prefix* p=(Prefix*)expression;
            return operator_predict_result(t_null, expression_object_type(p->right), p->op)!=t_any;
        }
        default:
            return false;
    }
}

Expression* to_literal(Executor* E, Object o){
    switch(o.type){
        case t_string:
        {
            StringLiteral* l=new_string_literal();
            l->value=strdup(o.text);
            return (Expression*)l;
        }
        case t_int:
        {
            IntLiteral* l=new_int_literal();
            l->value=o.int_value;
            return (Expression*)l;
        }
        case t_float:
        {
            FloatLiteral* l=new_float_literal();
            l->value=o.float_value;
            return (Expression*)l;
        }
        case t_null:
            return (Expression*)new_empty();
        case t_table:
            handle_if_error(E, o);
            return NULL;
        default:
            return NULL;
    }
}

Object evaluate_expression(Executor* E, Expression* expression){
    return execute_ast(E, expression, false);
}

ASTVisitorRequest optimise_ast_visitor (Expression* expression, void* data){
    Executor* E=(Executor*)data;

    // remove statements that have no side effects and whose result isn't used anywhere
    if(expression->type==e_block){
        Block* b=(Block*)expression;
        if(vector_count(&b->lines)==1){
            Expression* line=pointers_vector_get(&b->lines, 0);
            if(line->type!=e_assignment){
                ASTVisitorRequest request={next, copy_expression(line)};
                LOG_CHANGE("replacing one line block with this one line", expression, request.replacement);
                return request;
            }
        }
        for (int i = 0; i < vector_count(&b->lines)-1; i++){// last line isn't optimised because it is a result of the block
            Expression* line=pointers_vector_get(&b->lines, i);
            if(expression_is_constant(line)){
                USING_STRING(stringify_expression(line, 0),
                    printf("\ndeleting useless statement:%s\n", str));
                delete_expression(line);
                vector_delete(&b->lines, i);
                ASTVisitorRequest request={down};
                return request;
            }
        }
    }
    // if conditional condition is constant replace it with corresponding branch
    else if(expression->type==e_conditional){
        Conditional* c=(Conditional*)expression;
        if(expression_is_constant(c->condition)){
            Object evaluated=evaluate_expression(E, c->condition);
            handle_if_error(E, evaluated);
            ASTVisitorRequest request={next};
            if(is_falsy(evaluated)){
                request.replacement=copy_expression(c->onfalse);
            } else {
                request.replacement=copy_expression(c->ontrue);
            }
            LOG_CHANGE("constant conditional", expression, request.replacement);
            return request;
        }
    }
    // constants folding
    else if(!expression_is_literal(expression) && expression_is_constant(expression)){
        ASTVisitorRequest request={next};
        Object evaluated=evaluate_expression(E, expression);
        request.replacement=to_literal(E, evaluated);
        //replace current expression with the literal
        LOG_CHANGE("constants folding", expression, request.replacement);
        return request;
    }
    ASTVisitorRequest request={down};
    return request;
}

void optimise_ast(Executor* E, Expression** ast){
    visit_ast(ast, optimise_ast_visitor, E);
}

#undef LOG_CHANGE