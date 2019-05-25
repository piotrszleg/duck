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
    ast_visitor_request request={down, NULL};
    postprocessing_state* state=data;

    // translate messages to functions
    if(exp->type==e_message){
        message* m=(message*)exp;
        function_call* c=new_function_call();
        c->called=copy_expression(m->messaged_object);
        if(c->called->type!=e_path){
            THROW_ERROR(INCORRECT_OBJECT_POINTER, "The messaged_object field of message should be of type path.");
        }
        path* p=new_path();
        p->column_number=m->column_number;
        p->line_number=m->line_number;
        vector_init(&p->lines);
        vector_add(&p->lines, copy_expression(m->message_name));
        vector_add(&((path*)c->called)->lines, copy_expression(m->message_name));

        c->arguments=new_block();
        vector_init(&c->arguments->lines);
        vector_add(&c->arguments->lines, copy_expression(m->messaged_object));
        for(int i=0; i<vector_total(&m->arguments->lines); i++){
            vector_add(&c->arguments->lines, copy_expression(vector_get(&m->arguments->lines, i)));
        }
        c->column_number=m->column_number;
        c->line_number=m->line_number;

        request.replacement=c;
        return request;
    }
    // ast_vistor entered a function
    if(exp->type==e_function_declaration){
        state->current_function=exp;
    }
    if(exp->type==e_assignment){
        assignment* a=(assignment*)exp;
        char* variable=get_variable_name(a->left);
        if(variable!=NULL){
            // first assignment to variable is it's declaration
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
    return request;
}

void postprocess_ast(expression* ast){
    postprocessing_state state;
    state.current_function=NULL;
    map_init(&state.declarations);
    
    visit_ast(ast, postprocess_ast_visitor, &state);
    map_deinit(&state.declarations);
}