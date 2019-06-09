#include "macros.h"

typedef struct {
    Executor* executor;
    map_t(Object) macro_definitions;
} MacroVisitorState;

char* path_to_string(path* p){
    stream s;
    init_stream(&s, 32);
    for(int i=0; i<vector_total(&p->lines); i++){
        expression* line=vector_get(&p->lines, i);
        if(line->type!=e_name){
            USING_STRING(stringify_expression(line, 0),
                THROW_ERROR(AST_ERROR, "Macro path can only consist of names, %s given.", str))
        }
        char* name_value=((name*)line)->value;
        stream_push(&s, name_value, strlen(name_value));
    }
    return stream_get_data(&s);
}

expression* to_literal(Object o);
Object evaluate_expression(Executor* E, expression* exp);

expression* evaluate_macro(Executor* E, Object macro_value, Object* arguments, int arguments_count) {
    if(macro_value.type!=t_function){
        return to_literal(macro_value);
    } else {
        Object call_result=call(E, macro_value, arguments, arguments_count);
        if(call_result.type==t_table && !is_falsy(get(E, call_result, to_string("is_expression")))){
            return (expression*)get(E, call_result, to_string("position")).p;
        } else {
            return to_literal(call_result);
        }
    }
}

int macro_arguments_count(Object macro_value){
    if(macro_value.type==t_function){
        return macro_value.fp->arguments_count;
    } else {
        return 0;
    }
}

Object wrap_expression(Executor* E, expression* exp){
    return evaluate_expression(E, exp);
}

void proccess_macro_declaration(macro_declaration* md, MacroVisitorState* state){
    char* key=path_to_string(md->left->pth);
        map_set(&state->macro_definitions, key, evaluate_expression(state->executor, md->right));
        free(key);
}

ast_visitor_request macro_visitor(expression* exp, void* data){
    MacroVisitorState* state=(MacroVisitorState*)data;
    if(exp->type==e_macro_declaration){
        proccess_macro_declaration((macro_declaration*)exp, state);
        ast_visitor_request request={next, NULL};
        return request;
    } else if(exp->type==e_block){
        block* b=(block*)exp;
        for(int i=0; i<vector_total(&b->lines); i++){
            expression* line=vector_get(&b->lines, i);
            if(line->type==e_macro_declaration){
                proccess_macro_declaration((macro_declaration*)line, state);
            } else if(line->type==e_macro){
                macro* m=(macro*)line;
                char* key=path_to_string(m->pth);

                Object* map_get_result=map_get(&state->macro_definitions, key);
                if(map_get_result==NULL){
                    THROW_ERROR(AST_ERROR, "Unknown macro %s.", key)
                }
                Object macro_value=*map_get_result;
                
                int expected_arguments=macro_arguments_count(macro_value);
                if(expected_arguments>vector_total(&b->lines)-i-1){
                    USING_STRING(stringify_expression(exp, 0),
                        THROW_ERROR(AST_ERROR, "There is not enough lines in block: %s for %s macro to work.", str, key))
                }
                Object* arguments=malloc(sizeof(Object)*expected_arguments);
                for(int j=0; j<expected_arguments; j++){
                    arguments[j]=wrap_expression(state->executor, vector_get(&b->lines, i+1+j));
                }
                expression* evaluation_result=evaluate_macro(state->executor, macro_value, arguments, expected_arguments);
                if(evaluation_result!=NULL) {
                    block* copy=new_block();
                    copy->line_number=b->line_number;
                    copy->column_number=b->column_number;
                    for(int j=0; j<vector_total(&b->lines); j++){
                        if(j==i){
                            vector_add(&copy->lines, evaluation_result);
                            j+=expected_arguments;
                        } else {
                            vector_add(&copy->lines, copy_expression(vector_get(&b->lines, j)));
                        }
                    }
                    free(key);
                    ast_visitor_request request={down, (expression*)copy};
                    return request;
                } else {
                    free(key);
                    THROW_ERROR(AST_ERROR, "Evaluation of macro %s failed.", key)
                }
            }
        }
        ast_visitor_request request={down};
        return request;
    } else if(exp->type==e_macro){
        macro* m=(macro*)exp;
        char* key=path_to_string(m->pth);
        Object* map_get_result=map_get(&state->macro_definitions, key);
        if(map_get_result==NULL){
            THROW_ERROR(AST_ERROR, "Unknown macro %s.", key)
        }
        ast_visitor_request request={next,
            evaluate_macro(state->executor, *map_get_result, NULL, 0)};
        free(key);
        return request;
    } else {
        ast_visitor_request request={down, NULL};
        return request;
    }
}

void execute_macros(Executor* E, expression* ast){
    MacroVisitorState state;
    state.executor=E;
    map_init(&state.macro_definitions);
    visit_ast(ast, macro_visitor, &state);
    map_deinit(&state.macro_definitions);
}