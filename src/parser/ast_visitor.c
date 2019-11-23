#include "ast_visitor.h"

ASTVisitorRequest call_for_replacements(Expression** expression, visitor_function f, void* data){
    ASTVisitorRequest request=f(*expression, data);

    // we want to pass the last replacement that isn't NULL
    Expression* replacement=request.replacement;
    while(request.replacement){
        // call visitor function on replacing expression
        request=visit_ast(&request.replacement, f, data);
        if(request.replacement!=NULL){
            replacement=request.replacement;
        }
    }
    request.replacement=replacement;
    return request;
}

ASTVisitorRequest visit_ast(Expression** expression, visitor_function f, void* data){
    ASTVisitorRequest request=call_for_replacements(expression, f, data);
    if(request.replacement!=NULL){
        delete_expression(*expression);
        *expression=request.replacement;
        return request;
    }

    if(request.move!=down){
        return request;
    }
    // visit subexpressions:
    switch((*expression)->type){
        #define EXPRESSION(struct_name, type_tag) \
            case e_##type_tag: {\
            struct_name* casted=(struct_name*)*expression; \
            allow_unused_variable(casted);
        #define SUBEXPRESSION(type, e) {\
            ASTVisitorRequest subexpression_request=visit_ast((Expression**)&(e), f, data); \
            if(subexpression_request.move==up){ \
                break; \
            } \
        }
        #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) SUBEXPRESSION(type, casted->field_name);
        #define EXPRESSION_FIELD(field_name)                                  SUBEXPRESSION(expression, casted->field_name);
        #define BOOL_FIELD(field_name)
        #define STRING_FIELD(field_name)
        #define FLOAT_FIELD(field_name)
        #define INT_FIELD(field_name)
        #define VECTOR_FIELD(field_name) \
            for (int i = 0; i < vector_count(&casted->field_name); i++){ \
                Expression** line=vector_index(&casted->field_name, i); \
                ASTVisitorRequest subexpression_request=visit_ast(line, f, data); \
                if(subexpression_request.move==up){ \
                    break; \
                } \
            }
        #define END break; }
        default: INCORRECT_ENUM_VALUE(ExpressionType, expression, (*expression)->type);

        AST_EXPRESSIONS

        #undef SUBEXPRESSION
        #undef EXPRESSION
        #undef SPECIFIED_EXPRESSION_FIELD
        #undef EXPRESSION_FIELD               
        #undef BOOL_FIELD                   
        #undef STRING_FIELD
        #undef VECTOR_FIELD
        #undef FLOAT_FIELD
        #undef INT_FIELD
        #undef END
    }
    // expressions changing the scope are called two times to allow detecting this change
    if((*expression)->type==e_function_declaration || (*expression)->type==e_block || (*expression)->type==e_table_literal){
        request=call_for_replacements(expression, f, data);
    }
    return request;
}