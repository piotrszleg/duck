#include "ast_postprocessing.h"

/*
have a list of declarations with their names and depth and expression pointer
every time you have new declaration add it to the list
if a there is a reference that is deeper than its corresponding declaration then the declaration is a closure
*/

typedef struct {
    function_declaration* owning_function;
    assignment* first_assignment;
} declaration;

typedef struct {
    map_t(declaration) declarations;
    function_declaration* current_function;
} postprocessing_state;

char* get_variable_name(path* p){
    if(vector_total(&p->lines)!=1){
        return NULL;
    }
    expression* path_first=(expression*)vector_get(&p->lines, 0);
    if(path_first->type!=e_name){
        return NULL;
    }
    return ((name*)path_first)->value;
}

ast_visitor_request postprocess_ast_visitor(expression* exp, void* data){
    postprocessing_state* state=data;
    if(exp->type==e_function_declaration){
        state->current_function=exp;
    }
    if(exp->type==e_assignment){
        assignment* a=(assignment*)exp;
        char* variable=get_variable_name(a->left);
        if(variable!=NULL){
            if(map_get(&state->declarations, variable)==NULL){
                declaration decl={state->current_function, a};
                map_set(&state->declarations, variable, decl);
            }
        }
    }
    if(exp->type==e_path){
        char* variable=get_variable_name((path*)exp);
        if(variable!=NULL){
            declaration* decl=map_get(&state->declarations, variable);
            if(decl!=NULL && decl->owning_function!=state->current_function){
                decl->first_assignment->used_in_closure=true;
            }
        }
    }
    ast_visitor_request request={down, NULL};
    return request;
}

void postprocess_ast(expression* ast){
    postprocessing_state state;
    state.current_function=NULL;
    map_init(&state.declarations);
    
    visit_ast(ast, postprocess_ast_visitor, &state);
    map_deinit(&state.declarations);
}