#include "ast_postprocessing.h"

/*
have a list of declarations with their names and depth and expression pointer
every time you have new declaration add it to the list
if a there is a reference that is deeper than its corresponding declaration then the declaration is a closure
*/

typedef struct {
    function_declaration* owning_function;
    expression* first_assignment;
} VariableDeclaration;

typedef struct {
    map_t(VariableDeclaration) declarations;
    vector functions;
} PostprocessingState;

char* get_variable_name(path* p){
    if(vector_count(&p->lines)!=1){
        return NULL;
    }
    expression* path_first=(expression*)pointers_vector_get(&p->lines, 0);
    if(path_first->type!=e_name){
        return NULL;
    }
    return ((name*)path_first)->value;
}

ASTVisitorRequest postprocess_ast_visitor(expression* exp, void* data){
    ASTVisitorRequest request={down, NULL};
    PostprocessingState* state=data;

    switch(exp->type){
    
    // translate messages to functions
    case e_message:{
        message* m=(message*)exp;
        function_call* c=new_function_call();
        c->called=copy_expression(m->messaged_object);
        if(c->called->type!=e_path){
            THROW_ERROR(INCORRECT_OBJECT_POINTER, "The messaged_object field of message should be of type path.");
        }
        pointers_vector_push(&((path*)c->called)->lines, copy_expression((expression*)m->message_name));

        c->arguments=new_table_literal();
        pointers_vector_push(&c->arguments->lines, copy_expression(m->messaged_object));
        for(int i=0; i<vector_count(&m->arguments->lines); i++){
            pointers_vector_push(&c->arguments->lines, copy_expression(pointers_vector_get(&m->arguments->lines, i)));
        }
        c->column_number=m->column_number;
        c->line_number=m->line_number;

        request.replacement=(expression*)c;
        return request;
    }
    // changing order of operations from right to left to left to right
    case e_binary: {
        binary* b=(binary*)exp;
        if(b->right->type==e_binary){
            binary* b2=(binary*)b->right;

            binary* replacement=new_binary();
            replacement->left=copy_expression(b->left);
            replacement->right=copy_expression(b2->left);
            replacement->op=strdup(b->op);
            replacement->line_number=b->line_number;
            replacement->column_number=b->column_number;

            binary* replacement_2=new_binary();
            replacement_2->left=(expression*)replacement;
            replacement_2->right=copy_expression(b2->right);
            replacement_2->op=strdup(b2->op);
            replacement_2->line_number=b2->line_number;
            replacement_2->column_number=b2->column_number;

            request.replacement=(expression*)replacement_2;
            return request;
        }
        break;
    }
    case e_function_declaration: {
        // if the function is already on the stack then ast_visitor is escaping it
        if(*(expression**)(vector_top(&state->functions))==exp) {
            vector_pop(&state->functions);
        } else {
            // ast_visitor entered this function
            vector_push(&state->functions, (const void*)&exp);
        }
        break;
    }
    case e_argument: {
        argument* a=(argument*)exp;
        char* variable=a->name;
        if(map_get(&state->declarations, variable)==NULL){
            VariableDeclaration decl={(function_declaration*)vector_top(&state->functions), (expression*)a};
            map_set(&state->declarations, variable, decl);
        }
        break;
    }
    case e_assignment: {
        assignment* a=(assignment*)exp;
        char* variable=get_variable_name(a->left);
        if(variable!=NULL){
            // first assignment to variable is it's declaration
            if(map_get(&state->declarations, variable)==NULL){
                VariableDeclaration decl={(function_declaration*)vector_top(&state->functions), (expression*)a};
                map_set(&state->declarations, variable, decl);
            }
        }
        break;
    }
    case e_path: {
        char* variable=get_variable_name((path*)exp);
        if(variable!=NULL){
            VariableDeclaration* decl=map_get(&state->declarations, variable);
            if(decl!=NULL && decl->owning_function!=(function_declaration*)vector_top(&state->functions)){
                if(decl->first_assignment->type==e_assignment){
                    ((assignment*)decl->first_assignment)->used_in_closure=true;
                } else if(decl->first_assignment->type==e_argument) {
                    ((argument*)decl->first_assignment)->used_in_closure=true;
                }
            }
        }
        break;
    }
    default:;
    }
    return request;
}

void postprocess_ast(expression* ast){
    PostprocessingState state;
    vector_init(&state.functions, sizeof(function_declaration*), 16);
    vector_push(&state.functions, ast);
    map_init(&state.declarations);
    
    visit_ast(ast, postprocess_ast_visitor, &state);
    map_deinit(&state.declarations);
}