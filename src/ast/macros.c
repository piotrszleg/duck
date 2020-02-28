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
        if(macro_arguments_count(macro_value)>0){
            MACRO_ERROR(null_const, 
                "There is not enough lines in block to fill \"%s\" macro's required arguments.", macro_name);
        }
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

typedef struct {
    Executor* executor;
    Table* variables_set;
} QuoteWithVariablesVisitorState;

static bool table_contains_string(Executor* E, Table* table, char* key){
    TableIterator it=table_get_iterator(table);
    for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
        if(EQUALS_STRING(i.value, key)){
            return true;
        }
    }
    return false;
}

ASTVisitorRequest quote_with_variables_visitor(Expression* expression, void* data){
    QuoteWithVariablesVisitorState* state=(QuoteWithVariablesVisitorState*)data;
    if(expression->type==e_table_literal){
        // ['macro, identifier=['name, value='variable]]
        TableLiteral* as_table_literal=(TableLiteral*)expression;
        Expression* first_line=pointers_vector_get(&as_table_literal->lines, 0);
        if(first_line->type==e_string_literal){
            // 'macro
            StringLiteral* as_string_literal=(StringLiteral*)first_line;
            if(strcmp(as_string_literal->value, "macro")==0){
                // identifier=['name, value='variable]
                Expression* second_line=pointers_vector_get(&as_table_literal->lines, 1);
                Assignment* second_line_assignment=(Assignment*)second_line;
                // ['name, value='variable]
                TableLiteral* macro_name=(TableLiteral*)second_line_assignment->right;
                // value='variable
                Assignment* macro_name_value_assignment=(Assignment*)pointers_vector_get(&macro_name->lines, 1);
                // 'variable
                StringLiteral* name_string_literal=(StringLiteral*)macro_name_value_assignment->right;
                if(table_contains_string(state->executor, state->variables_set, name_string_literal->value)){
                    Name* name=new_name();
                    name->value=strdup(name_string_literal->value);
                    ASTVisitorRequest request={next, (Expression*)name};
                    return request;
                }
            }
        }
    }
    ASTVisitorRequest request={down, NULL};
    return request;
}

Object quote_with_macro(Executor* E, Object scope, Object* arguments, int arguments_count){
    Expression* expression=object_to_expression(E, arguments[0]);
    Object variables_set=evaluate(E, object_to_expression(E, arguments[1]), E->scope, "quote_macro", true);
    
    Object as_object=expression_to_object(E, expression);
    Expression* as_literal=object_to_literal(E, as_object);
    QuoteWithVariablesVisitorState state={E, variables_set.tp};
    visit_ast(&as_literal, quote_with_variables_visitor, &state);
    Object literal_expression_object=expression_to_object(E, as_literal);
    
    delete_expression(expression);
    dereference(E, &as_object);
    delete_expression(as_literal);
    dereference(E, &variables_set);
    return literal_expression_object;
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