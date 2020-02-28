#include "macros.h"

typedef struct {
    Executor* executor;
    Object error;
} MacroVisitorState;

Object evaluate_macro(Executor* E, Object macro_value, Object* arguments, int arguments_count) {
    if(macro_value.type!=t_function){
        return macro_value;
    } else {
        return call(E, macro_value, arguments, arguments_count);
    }
}

int macro_arguments_count(Object macro_value){
    if(macro_value.type==t_function){
        return macro_value.fp->arguments_count;
    } else {
        return 0;
    }
}

void proccess_macro_declaration(MacroDeclaration* md, MacroVisitorState* state){
    Executor* E=state->executor;
    Object scope=E->scope;
    table_set(E, state->executor->scope.tp, to_string(md->left->identifier->value), 
              evaluate(E, md->right, scope, "macro", false));
}

ASTVisitorRequest macro_visitor(Expression* expression, void* data){
    MacroVisitorState* state=(MacroVisitorState*)data;
    Executor* E=state->executor;
    #define MACRO_ERROR(cause, message, ...) \
    {   NEW_ERROR(state->error, "MACRO_ERROR", cause, message, ##__VA_ARGS__) \
        ASTVisitorRequest request={down, NULL}; \
        return request; }
    #define GET_MACRO(result, macro_name, macro_expression) \
    {   macro_name=((Macro*)macro_expression)->identifier->value; \
        result=get(E, E->scope, to_string(macro_name)); \
        if(result.type==t_null){ \
            MACRO_ERROR(null_const, "Undefined macro \"%s\".", macro_name) \
        }   }

    if(expression->type==e_macro_declaration){
        proccess_macro_declaration((MacroDeclaration*)expression, state);
        ASTVisitorRequest request={next, NULL};
        return request;
    } else if(expression->type==e_block){
        Block* b=(Block*)expression;
        for(int i=0; i<vector_count(&b->lines); i++){
            Expression* line=pointers_vector_get(&b->lines, i);
            if(line->type==e_macro_declaration){
                proccess_macro_declaration((MacroDeclaration*)line, state);
            } else if(line->type==e_macro){
                Object macro_value;
                char* macro_name;
                GET_MACRO(macro_value, macro_name, line)
                
                int expected_arguments=macro_arguments_count(macro_value);
                if(expected_arguments>vector_count(&b->lines)-i-1){
                    MACRO_ERROR(null_const, 
                        "There is not enough lines in block to fill \"%s\" macro's required arguments.", macro_name);
                }
                Object* arguments=malloc(sizeof(Object)*expected_arguments);
                for(int j=0; j<expected_arguments; j++){
                    arguments[j]=expression_to_object(E, copy_expression(pointers_vector_get(&b->lines, i+1+j)));\
                }
                Object evaluation_result=evaluate_macro(E, macro_value, arguments, expected_arguments);
                Expression* converted=object_to_expression(E, evaluation_result);
                if(converted!=NULL) {
                    // replace macro with it's evaluation result
                    pointers_vector_set(&b->lines, i, converted);
                    delete_expression(line);
                    // remove its arguments
                    for(int j=0; j<expected_arguments; j++){
                        delete_expression(pointers_vector_get(&b->lines, i+1));
                        dereference(E, &arguments[j]);
                        vector_delete(&b->lines, i+1);
                    }
                    dereference(E, &evaluation_result);
                    ASTVisitorRequest request={down};
                    return request;
                } else {
                    MACRO_ERROR(evaluation_result, 
                        "Can't turn the result of evaluating macro \"%s\" back to expression.", macro_name)
                }
            }
        }
    } else if(expression->type==e_macro){
        Object macro_value;
        char* macro_name;
        GET_MACRO(macro_value, macro_name, expression)
        Object evaluation_result=evaluate_macro(E, macro_value, NULL, 0);
        Expression* converted=object_to_expression(E, evaluation_result);
        if(converted!=NULL) {
            ASTVisitorRequest request={next, converted};
            return request;
        } else {
            MACRO_ERROR(evaluation_result, 
                "Can't turn the result of evaluating macro \"%s\" back to expression.", macro_name)
        }
    }
    #undef MACRO_ERROR
    #undef GET_MACRO
    ASTVisitorRequest request={down, NULL};
    return request;
}
Object quote_with_macro(Executor* E, Object scope, Object* arguments, int arguments_count){
    Expression* expression=object_to_expression(E, arguments[0]);
    Object macros_map=evaluate(E, object_to_expression(E, arguments[1]), E->scope, "quote_macro", true);
    MacroVisitorState state;
    state.error=null_const;
    state.executor=E;
    Object sub_scope;
    table_init(E, &sub_scope);
    TableIterator it=table_get_iterator(macros_map.tp);
    for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
        table_set(E, sub_scope.tp, i.key, i.value);
    }
    E->scope=sub_scope;
    visit_ast(&expression, macro_visitor, &state);
    pop_return_point(E);
    if(state.error.type!=t_null){
        return state.error;
    } else {
        Object as_object=expression_to_object(E, expression);
        Expression* as_literal=object_to_literal(E, as_object);
        Object literal_expression_object=expression_to_object(E, as_literal);
        delete_expression(expression);
        dereference(E, &as_object);
        delete_expression(as_literal);
        return literal_expression_object;
    }
}
Object quote_macro(Executor* E, Object scope, Object* arguments, int arguments_count){
    Expression* as_literal=object_to_literal(E, arguments[0]);
    Object literal_expression_object=expression_to_object(E, as_literal);
    delete_expression(as_literal);
    return literal_expression_object;
}
Object execute_macros(Executor* E, Expression** ast){
    MacroVisitorState state;
    state.error=null_const;
    state.executor=E;
    create_return_point(E, true);
    Object scope;
    table_init(E, &scope);
    inherit_global_scope(E, scope.tp);
    set_function(E, scope, to_string("quote_with"), 2, false, quote_with_macro);
    set_function(E, scope, to_string("quote"), 1, false, quote_macro);
    E->scope=scope;
    visit_ast(ast, macro_visitor, &state);
    pop_return_point(E);
    return state.error;
}