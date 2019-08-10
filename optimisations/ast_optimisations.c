#include "ast_optimisations.h"

#define LOG_CHANGE(message, before, after) \
    if(E->options.print_ast_optimisations){ \
        char* before_string=stringify_expression(before, 0); \
        char* after_string=stringify_expression(after, 0); \
        printf("\n%s\nreplacing:%s\nwith:%s\n", message, before_string, after_string); \
        free(before_string); \
        free(after_string); \
    }

bool expression_is_literal(expression* exp){
    switch(exp->type){
        case e_empty:
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

ObjectTypeOrUnknown expression_object_type(expression* exp){
    switch(exp->type){
        case e_empty: return tu_null;
        case e_int_literal: return tu_int;
        case e_float_literal: return tu_float;
        case e_string_literal: return tu_string;
        case e_table_literal: return tu_table;
        case e_function_declaration: return tu_function;
        default: return tu_unknown;
    }
}

bool expression_is_constant(expression* exp){
    if(expression_is_literal(exp)){
        return true;
    }
    switch(exp->type){
        case e_binary:
        {
            binary* b=(binary*)exp;
            return operator_predict_result(expression_object_type(b->left), expression_object_type(b->right), b->op)!=tu_unknown;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            return operator_predict_result(tu_null, expression_object_type(p->right), p->op)!=tu_unknown;
        }
        default:
            return false;
    }
}

expression* to_literal(Executor* E, Object o){
    switch(o.type){
        case t_string:
        {
            string_literal* l=new_string_literal();
            l->value=strdup(o.text);
            return (expression*)l;
        }
        case t_int:
        {
            int_literal* l=new_int_literal();
            l->value=o.int_value;
            return (expression*)l;
        }
        case t_float:
        {
            float_literal* l=new_float_literal();
            l->value=o.float_value;
            return (expression*)l;
        }
        case t_null:
            return (expression*)new_empty();
        case t_table:
            handle_if_error(E, o);
            return NULL;
        default:
            return NULL;
    }
}

Object evaluate_expression(Executor* E, expression* exp){
    Object scope;
    table_init(E, &scope);
    return execute_ast(E, exp, scope, 0);
}

ASTVisitorRequest optimise_ast_visitor (expression* exp, void* data){
    Executor* E=(Executor*)data;

    // remove statements that have no side effects and whose result isn't used anywhere
    if(exp->type==e_block){
        block* b=(block*)exp;
        if(vector_count(&b->lines)==1){
            expression* line=pointers_vector_get(&b->lines, 0);
            if(line->type!=e_assignment){
                ASTVisitorRequest request={next, copy_expression(line)};
                LOG_CHANGE("replacing one line block with this one line", exp, request.replacement);
                return request;
            }
        }
        for (int i = 0; i < vector_count(&b->lines)-1; i++){// last line isn't optimised because it is a result of the block
            expression* line=pointers_vector_get(&b->lines, i);
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
    else if(exp->type==e_conditional){
        conditional* c=(conditional*)exp;
        if(expression_is_constant(c->condition)){
            Object evaluated=evaluate_expression(E, c->condition);
            handle_if_error(E, evaluated);
            ASTVisitorRequest request={next};
            if(is_falsy(evaluated)){
                request.replacement=copy_expression(c->onfalse);
            } else {
                request.replacement=copy_expression(c->ontrue);
            }
            LOG_CHANGE("constant conditional", exp, request.replacement);
            return request;
        }
    }
    // constants folding
    else if(!expression_is_literal(exp) && expression_is_constant(exp)){
        ASTVisitorRequest request={next};
        Object evaluated=evaluate_expression(E, exp);
        request.replacement=to_literal(E, evaluated);
        //replace current expression with the literal
        LOG_CHANGE("constants folding", exp, request.replacement);
        return request;
    }
    ASTVisitorRequest request={down};
    return request;
}

void optimise_ast(Executor* E, expression** ast){
    visit_ast(ast, optimise_ast_visitor, E);
}

#undef LOG_CHANGE