#include "ast_optimisations.h"

#define LOG_CHANGE(message, before, after) \
    if(Ex->opt.print_ast_optimisations){ \
        char* before_string=stringify_expression(before, 0); \
        char* after_string=stringify_expression(after, 0); \
        printf("\n%s\nreplacing:%s\nwith:%s\n", message, before_string, after_string); \
        free(before_string); \
        free(after_string); \
    }

bool is_literal(expression* exp){
    switch(exp->type){
        case e_empty:
        case e_literal:
            return true;
        default:
            return false;
    }
}

bool is_constant(expression* exp){
    switch(exp->type){
        case e_empty:
        case e_literal:
            return true;
        case e_binary:
        {
            binary* u=(binary*)exp;
            return is_constant(u->left) && is_constant(u->right);
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            return is_constant(p->right);
        }
        default:
            return false;
    }
}

expression* to_literal(object o){
    if(o.type==t_null){
        return (expression*)new_empty();
    }
    
    switch(o.type){
        case t_string:
        {
            literal* l=new_literal();
            l->sval=strdup(o.text);
            l->ltype=l_string;
            return (expression*)l;
        }
        case t_number:
        {
            literal* l=new_literal();
            l->fval=o.value;
            l->ltype=l_float;
            return (expression*)l;
        }
        case t_null:
            return (expression*)new_empty();
        default:
            return NULL;
    }
}

object evaluate_expression(executor* Ex, expression* exp){
    object scope;
    table_init(&scope);
    object execution_result=execute_ast(Ex, exp, scope, 0);
    dereference(Ex, &scope);
    return execution_result;
}

ast_visitor_request optimise_ast_visitor (expression* exp, void* data){
    executor* Ex=(executor*)data;

    // remove statements that have no side effects and whose result isn't used anywhere
    if(exp->type==e_block){
        block* b=(block*)exp;
        if(vector_total(&b->lines)==1){
            expression* line=vector_get(&b->lines, 0);
            if(line->type!=e_assignment){
                ast_visitor_request request={next, copy_expression(line)};
                LOG_CHANGE("replacing one line block with this one line", exp, request.replacement);
                return request;
            }
        }
        for (int i = 0; i < vector_total(&b->lines)-1; i++){// last line isn't optimised because it is a result of the block
            expression* line=vector_get(&b->lines, i);
            if(is_constant(line)){
                USING_STRING(stringify_expression(line, 0),
                    printf("\ndeleting useless statement:%s\n", str));
                delete_expression(line);
                vector_delete(&b->lines, i);
                ast_visitor_request request={down};
                return request;
            }
        }
    }
    // if conditional condition is constant replace it with corresponding branch
    else if(exp->type==e_conditional){
        conditional* c=(conditional*)exp;
        if(is_constant(c->condition)){
            object evaluated=evaluate_expression(Ex, c->condition);
            ast_visitor_request request={next};
            if(is_falsy(evaluated)){
                request.replacement=copy_expression(c->onfalse);
            } else {
                request.replacement=copy_expression(c->ontrue);
            }
            LOG_CHANGE("constant conditional", exp, request.replacement);
            dereference(Ex, &evaluated);
            return request;
        }
    }
    // constants folding
    else if(!is_literal(exp) && is_constant(exp)){
        ast_visitor_request request={next};
        object evaluated=evaluate_expression(Ex, exp);
        request.replacement=to_literal(evaluated);
        //replace current expression with the literal
        dereference(Ex, &evaluated);
        LOG_CHANGE("constants folding", exp, request.replacement);
        return request;
    }
    ast_visitor_request request={down};
    return request;
}

void optimise_ast(executor* Ex, expression* ast){
    visit_ast(ast, optimise_ast_visitor, Ex);
}

#undef LOG_CHANGE