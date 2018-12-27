#include "ast_visitor.h"

ast_visitor_request visit_ast(expression* exp, visitor_function f, void* data){
    ast_visitor_request request=f(exp, data);
    if(request.move!=down){
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
                ast_visitor_request subexpression_request=visit_ast(line, f, data);
                if(subexpression_request.replacement){
                    printf("\nreplacing:\n[%s]\n with:\n[%s]", stringify_expression(line, 0), stringify_expression(subexpression_request.replacement, 0));
                    delete_expression(line);
                    vector_set(&b->lines, i, subexpression_request.replacement);
                }
                if(subexpression_request.move==up){
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
                ast_visitor_request subexpression_request=visit_ast(line, f, data);
                if(subexpression_request.replacement){
                    printf("replacing\n:[%s] with:\n[%s]", stringify_expression(line, 0), stringify_expression(subexpression_request.replacement, 0));
                    delete_expression(line);
                    vector_set(&c->arguments->lines, i, subexpression_request.replacement);
                }
                if(subexpression_request.move==up){
                    break;// skip rest of the lines
                }
            }
            break;
        }
        #define SUBEXPRESSION(e) \
            {ast_visitor_request subexpression_request=visit_ast((expression*)e, f, data); \
            if(subexpression_request.replacement) e=subexpression_request.replacement; }
        case _assignment:
        {
            assignment* a=(assignment*)exp;
            
            visit_ast((expression*)a->left, f, data);// left hand of assignment can't be changed
            SUBEXPRESSION(a->right)
            break;
        }
        case _unary:
        {
            unary* u=(unary*)exp;

            SUBEXPRESSION(u->right)
            SUBEXPRESSION(u->left)
            break;
        }
        case _prefix:
        {
            prefix* p=(prefix*)exp;
            SUBEXPRESSION(p->right)
            break;
        }
        case _conditional:
        {
            conditional* c=(conditional*)exp;

            SUBEXPRESSION(c->condition)
            SUBEXPRESSION(c->ontrue)
            SUBEXPRESSION(c->onfalse)
            break;
        }
        case _function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;
            SUBEXPRESSION(d->body)
            break;
        }
        case _function_return:
        {
            function_return* r=(function_return*)exp;
            SUBEXPRESSION(r->value)
            break;
        }
        #undef SUBEXPRESSION
        default: ;
    }
    return request;
}