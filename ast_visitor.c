#include "ast_visitor.h"

  visit_ast(expression* exp, visitor_function f, void* data){
    move_request request=f(exp, data);
    if(request!=down){
        return request;
    }
    switch(exp->type){
        case _block:
        case _table_literal:
        case _path:
        {
            block* b=(block*)exp;
            
            for (int i = 0; i < vector_total(&b->lines); i++){
                expression* line=vector_get(&b->lines, i);
                if(visit_ast_recursive(line, f, data)==up){
                    break;// skip rest of the lines
                }
            }
            break;
        }
        case _function_call:
        {
            function_call* c=(function_call*)exp;

            int lines_count=vector_total(&c->arguments->lines);
            for (int i = 0; i < lines_count; i++){
                expression* line=vector_get(&c->arguments->lines, i);
                if(visit_ast_recursive(line, f, data)==up){
                    break;
                }
            }
            break;
        }
        case _assignment:
        {
            assignment* a=(assignment*)exp;

            visit_ast_recursive(a->right, f, data);
            visit_ast_recursive(a->left, f, data);
            break;
        }
        case _unary:
        {
            unary* u=(unary*)exp;

            visit_ast_recursive(u->right, f, data);
            visit_ast_recursive(u->left, f, data);
            break;
        }
        case _prefix:
        {
            prefix* p=(prefix*)exp;
            visit_ast_recursive(p->right, f, data);
            break;
        }
        case _conditional:
        {
            conditional* c=(conditional*)exp;

            visit_ast_recursive(c->condition, f, data);
            visit_ast_recursive(c->ontrue, f, data);
            visit_ast_recursive(c->onfalse, f, data);
            break;
        }
        case _function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;
            visit_ast_recursive((expression*)d->body, f, data);
            break;
        }
        case _function_return:
        {
            function_return* r=(function_return*)exp;
            visit_ast_recursive(r->value, f, data);
            break;
        }
        default: ;
    }
}