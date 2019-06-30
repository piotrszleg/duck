#include "ast_postprocessing.h"

/*
Declarations map maps variable names in the current scope to their first assignment and owning function.
If a variable reference is found in a function that isn't it's owning_function then it's first_assignment used in closure.
*/
typedef struct {
    function_declaration* owning_function;
    expression* first_assignment;
} VariableDeclaration;

typedef map_t(VariableDeclaration) DeclarationsMap;

typedef struct {
    DeclarationsMap declarations;
    vector functions;
} PostprocessingState;

char* get_variable_name(path* p){
    if(vector_count(&p->lines)<1){
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
            function_declaration* f=vector_pop(&state->functions);
            // remove all variable declarations belonging to this function
            DeclarationsMap new_declarations;
            map_init(&new_declarations);
            map_iter_t iterator=map_iter(&state->declarations);
            const char* key;
            while((key=map_next(&state->declarations, &iterator))){
                VariableDeclaration* value=map_get(&state->declarations, key);
                if(value->owning_function!=f){
                    map_set(&new_declarations, key, *value);
                }
            }
            map_deinit(&state->declarations);
            state->declarations=new_declarations;
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

void postprocess_ast(expression** ast){
    PostprocessingState state;
    vector_init(&state.functions, sizeof(function_declaration*), 16);
    vector_push(&state.functions, ast);
    map_init(&state.declarations);
    
    visit_ast(ast, postprocess_ast_visitor, &state);
    map_deinit(&state.declarations);
}