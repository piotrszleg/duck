#include "ast_visitor.h"

ASTVisitorRequest call_for_replacements(expression* exp, visitor_function f, void* data){
    ASTVisitorRequest request=f(exp, data);

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

ASTVisitorRequest visit_ast(expression* exp, visitor_function f, void* data){
    ASTVisitorRequest request=call_for_replacements(exp, f, data);

    if(request.replacement!=NULL || request.move!=down){
        return request;
    }

    switch(exp->type){
        #define EXPRESSION(type) \
            case e_##type: {\
            type* casted=(type*)exp; \
            allow_unused_variable(casted);
        #define SUBEXPRESSION(type, e) {\
            ASTVisitorRequest subexpression_request=visit_ast((expression*)(e), f, data); \
            if(subexpression_request.replacement!=NULL){ \
                delete_expression((expression*)(e)); \
                (e)=(type*)subexpression_request.replacement; \
            } \
            if(subexpression_request.move==up){ \
                break; \
            } \
        }
        #define SPECIFIED_EXPRESSION_FIELD(type, field_name) SUBEXPRESSION(type, casted->field_name);
        #define EXPRESSION_FIELD(field_name)                 SUBEXPRESSION(expression, casted->field_name);
        #define BOOL_FIELD(field_name)
        #define STRING_FIELD(field_name)
        #define FLOAT_FIELD(field_name)
        #define INT_FIELD(field_name)
        #define VECTOR_FIELD(field_name) \
            for (int i = 0; i < vector_count(&casted->field_name); i++){ \
                expression* line=pointers_vector_get(&casted->field_name, i); \
                ASTVisitorRequest subexpression_request=visit_ast(line, f, data); \
                if(subexpression_request.replacement){ \
                    delete_expression(line); \
                    pointers_vector_set(&casted->field_name, i, subexpression_request.replacement); \
                } \
                if(subexpression_request.move==up){ \
                    break;/* skip rest of the lines */ \
                } \
            }
        #define END break; }
        default: THROW_ERROR(AST_ERROR, "Incorrect expression type in vsit_ast function, type is %i", exp->type);

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
    // functions are visited two times to allow finding closures
    if(exp->type==e_function_declaration){
        request=call_for_replacements(exp, f, data);
    }
    return request;
}