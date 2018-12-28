#include "ast_optimisations.h"

bool is_constant(expression* exp){
    switch(exp->type){
        case e_literal:
            return true;
        case e_unary:
        {
            unary* u=(unary*)exp;
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
        return (expression*)newe_empty();
    }
    
    switch(o.type){
        case t_string:
        {
            literal* l=new_literal();
            l->sval=strdup(o.text);
            l->ltype=_string;
            return (expression*)l;
        }
        case t_number:
        {
            literal* l=new_literal();
            l->fval=o.value;
            l->ltype=_float;
            return (expression*)l;
        }
        case t_null:
            return (expression*)newe_empty();
        default:
            return NULL;
    }
}

ast_visitor_request optimise_ast_visitor (expression* exp, void* data){
    // remove statements that have no side effects and whose result isn't used anywhere
    if(exp->type==e_block){
        block* b=(block*)exp;
        for (int i = 0; i < vector_total(&b->lines)-1; i++){// last line isn't optimised because it is a result of the block
            if(is_constant(vector_get(&b->lines, i))){
                vector_delete(&b->lines, i);
            }
        }
    }
    // constants folding
    if(exp->type!=e_literal && is_constant(exp)){
        ast_executor_state state;
        object scope;
        table_init(&scope);
        object execution_result=execute_ast(&state, exp, scope, 0);
        ast_visitor_request request={next};
        request.replacement=to_literal(execution_result);
        //replace current expression with the literal
        object_deinit(&execution_result);
        object_deinit(&scope);
        return request;
    } else {
        ast_visitor_request request={down};
        return request;
    }
}

void optimise_ast(expression* ast){
    void* data=NULL;
    visit_ast(ast, optimise_ast_visitor, data);
}