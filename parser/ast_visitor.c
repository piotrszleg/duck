#include "ast_visitor.h"

ast_visitor_request call_for_replacements(expression* exp, visitor_function f, void* data){
    ast_visitor_request request=f(exp, data);

    // we want to pass the last replacement that isn't NULL
    expression* replacement=request.replacement;
    while(request.replacement){
        // call visitor function on replacing expression
        request=visit_ast(request.replacement, f, data);
        if(request.replacement!=NULL){
            replacement=request.replacement;
        }
    }
    request.replacement=replacement;
    return request;
}

ast_visitor_request visit_ast(expression* exp, visitor_function f, void* data){
    if(exp==NULL) {
        ast_visitor_request request={down};
        return request;
    }
    ast_visitor_request request=call_for_replacements(exp, f, data);

    if(request.replacement!=NULL || request.move!=down){
        return request;
    }

    switch(exp->type){
        case e_block:
        case e_table_literal:
        case e_path:
        {
            block* b=(block*)exp;
            
            for (int i = 0; i < vector_total(&b->lines); i++){
                expression* line=vector_get(&b->lines, i);
                if(line!=NULL){
                    ast_visitor_request subexpression_request=visit_ast(line, f, data);
                    if(subexpression_request.replacement){
                        delete_expression(line);
                        vector_set(&b->lines, i, subexpression_request.replacement);
                    }
                    if(subexpression_request.move==up){
                        break;// skip rest of the lines
                    }
                }
            }
            break;
        }
        #define SUBEXPRESSION(e) \
            if(e!=NULL){ \
                ast_visitor_request subexpression_request=visit_ast((expression*)e, f, data); \
                if(subexpression_request.replacement!=NULL){ \
                    delete_expression(e); \
                    e=subexpression_request.replacement; \
                } \
                if(subexpression_request.move==up){ \
                    break; \
                } \
            }
        case e_function_call:
        {
            function_call* c=(function_call*)exp;

            int lines_count=vector_total(&c->arguments->lines);
            for (int i = 0; i < lines_count; i++){
                expression* line=vector_get(&c->arguments->lines, i);
                if(line!=NULL){
                    ast_visitor_request subexpression_request=visit_ast(line, f, data);
                    if(subexpression_request.replacement!=NULL){
                        delete_expression(line);
                        vector_set(&c->arguments->lines, i, subexpression_request.replacement);
                    }
                    if(subexpression_request.move==up){
                        break;// skip rest of the lines
                    }
                }
            }
            break;
        }
        case e_message:
        {
            message* m=(message*)exp;

            int lines_count=vector_total(&m->arguments->lines);
            for (int i = 0; i < lines_count; i++){
                expression* line=vector_get(&m->arguments->lines, i);
                if(line!=NULL){
                    ast_visitor_request subexpression_request=visit_ast(line, f, data);
                    if(subexpression_request.replacement){
                        delete_expression(line);
                        vector_set(&m->arguments->lines, i, subexpression_request.replacement);
                    }
                    if(subexpression_request.move==up){
                        break;// skip rest of the lines
                    }
                }
            }
            break;
        }
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            
            visit_ast((expression*)a->left, f, data);// left hand of assignment can't be changed
            SUBEXPRESSION(a->right)
            break;
        }
        case e_binary:
        {
            binary* u=(binary*)exp;

            SUBEXPRESSION(u->right)
            SUBEXPRESSION(u->left)
            break;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            SUBEXPRESSION(p->right)
            break;
        }
        case e_conditional:
        {
            conditional* c=(conditional*)exp;

            SUBEXPRESSION(c->condition)
            SUBEXPRESSION(c->ontrue)
            SUBEXPRESSION(c->onfalse)
            break;
        }
        case e_function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;
            SUBEXPRESSION(d->body)

            // function declaration calls f two times to allow finding closures
            request=call_for_replacements(exp, f, data);
            break;
        }
        case e_function_return:
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