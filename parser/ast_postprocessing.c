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

char* get_root_variable_name(expression* exp){
    switch(exp->type){
        case e_name:
            return ((name*)exp)->value;
        case e_member_access:
            return get_root_variable_name(((member_access*)exp)->left);
        case e_null_conditional_member_access:
            return get_root_variable_name(((null_conditional_member_access*)exp)->left);
        case e_indexer:
            return get_root_variable_name(((indexer*)exp)->left);
        case e_null_conditional_indexer:
            return get_root_variable_name(((null_conditional_indexer*)exp)->left);
        default: 
            return NULL;
    }
}

bool is_null_conditional(expression* exp){
    return exp->type==e_null_conditional_indexer || exp->type==e_null_conditional_member_access;
}

expression* process_null_conditional(expression* dependency, expression* body){
    conditional* c=new_conditional();
    binary* b=new_binary();
    b->left=dependency;
    b->op=strdup("!=");
    b->right=(expression*)new_empty();
    c->condition=(expression*)b;
    c->ontrue=body;
    c->onfalse=(expression*)new_empty();
    return (expression*)c;
}

void replace_null_conditional(expression* exp) {
    switch(exp->type){
        #define EXPRESSION(type) \
            case e_##type: { \
                type* casted=(type*)exp; \
                allow_unused_variable((void*)casted);

        // null conditional expressions have same memory layout as their normal variants
        #define EXPRESSION_FIELD(field_name) \
            if(casted->field_name->type==e_null_conditional_member_access) { \
                casted->field_name->type=e_member_access; \
            } \
            if(casted->field_name->type==e_null_conditional_indexer) { \
                casted->field_name->type=e_indexer; \
            }
        #define SPECIFIED_EXPRESSION_FIELD(type, field_name) EXPRESSION_FIELD(field_name)
        #define BOOL_FIELD(field_name)
        #define STRING_FIELD(field_name)
        #define FLOAT_FIELD(field_name)
        #define INT_FIELD(field_name)
        #define VECTOR_FIELD(field_name)
        #define END \
                break; \
            }
        AST_EXPRESSIONS
        default:;

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
}

ASTVisitorRequest postprocess_ast_visitor(expression* exp, void* data){
    ASTVisitorRequest request={down, NULL};
    PostprocessingState* state=data;

    if(is_null_conditional(exp)) {
        expression* copy=copy_expression((expression*)exp);
        replace_null_conditional(copy);
        request.replacement=process_null_conditional(copy_expression((expression*)((null_conditional_member_access*)exp)->left), copy);
        return request;
    }
    switch(exp->type){
        #define EXPRESSION(type) \
            case e_##type: { \
                type* casted=(type*)exp; \
                allow_unused_variable((void*)casted);

        #define EXPRESSION_FIELD(field_name) \
            if(is_null_conditional((expression*)casted->field_name)) { \
                expression* copy=copy_expression((expression*)exp); \
                replace_null_conditional(copy); \
                request.replacement=process_null_conditional(copy_expression((expression*)((null_conditional_member_access*)casted->field_name)->left), copy); \
                return request; \
            }
        #define SPECIFIED_EXPRESSION_FIELD(type, field_name) EXPRESSION_FIELD(field_name)
        #define BOOL_FIELD(field_name)
        #define STRING_FIELD(field_name)
        #define FLOAT_FIELD(field_name)
        #define INT_FIELD(field_name)
        #define VECTOR_FIELD(field_name)
        #define END \
                break; \
            }
        AST_EXPRESSIONS
        default:;

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

    #define CURRENT_FUNCTION (*(function_declaration**)vector_top(&state->functions))
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
    case e_message: {
        /* 
            change message to function call

            messaged->message_name(arguments...)

            {
                messaged=messaged # save messaged to temporary variable to avoid executing it's statement two times
                messaged.message_name(messaged, arguments...)
            }
        */
        message* m=(message*)exp;
        
        name* messaged=new_name();
        messaged->value=strdup("messaged");

        // messaged=messaged
        assignment* messaged_assignment=new_assignment();
        messaged_assignment->left=(expression*)messaged;
        messaged_assignment->right=copy_expression(m->messaged_object);
        
        // messaged.message_name
        member_access* message_function=new_member_access();
        message_function->left=copy_expression((expression*)messaged);
        message_function->right=(name*)copy_expression((expression*)m->message_name);

        function_call* call=new_function_call();
        call->called=(expression*)message_function;
        // (messaged, arguments...)
        table_literal* call_arguments=new_table_literal();
        pointers_vector_push(&call_arguments->lines, copy_expression((expression*)messaged));
        for(int i=0; i<vector_count(&m->arguments->lines); i++) {
            pointers_vector_push(&call_arguments->lines, copy_expression((expression*)pointers_vector_get(&m->arguments->lines, i)));
        }
        call->arguments=call_arguments;

        // { }
        block* sub_scope=new_block();
        pointers_vector_push(&sub_scope->lines, messaged_assignment);
        pointers_vector_push(&sub_scope->lines, call);
        request.replacement=(expression*)sub_scope;
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
            VariableDeclaration decl={CURRENT_FUNCTION, (expression*)a};
            map_set(&state->declarations, variable, decl);
        }
        break;
    }
    case e_assignment: {
        assignment* a=(assignment*)exp;
        if(a->left->type==e_name) {
            char* variable_name=((name*)a->left)->value;
            // first assignment to variable is it's declaration
            if(map_get(&state->declarations, variable_name)==NULL){
                VariableDeclaration decl={CURRENT_FUNCTION, (expression*)a};
                map_set(&state->declarations, variable_name, decl);
            }
        }
        break;
    }
    case e_self_member_access: {
        self_member_access* sma=(self_member_access*)exp;
        member_access* ma=new_member_access();
        if(CURRENT_FUNCTION!=NULL && CURRENT_FUNCTION->type==e_function_declaration && vector_count(&CURRENT_FUNCTION->arguments)>0){
            name* argument_name=new_name();
            argument_name->value=strdup(((argument*)pointers_vector_get(&CURRENT_FUNCTION->arguments, 0))->name);
            ma->left=(expression*)argument_name;
        } else {
            ma->left=(expression*)new_empty();// in case it is used outside of a function
        }
        ma->right=(name*)copy_expression((expression*)sma->right);
        request.replacement=(expression*)ma;
        break;
    }
    case e_self_indexer: {
        self_indexer* si=(self_indexer*)exp;
        indexer* i=new_indexer();
        if(CURRENT_FUNCTION!=NULL && CURRENT_FUNCTION->type==e_function_declaration && vector_count(&CURRENT_FUNCTION->arguments)>0){
            name* argument_name=new_name();
            argument_name->value=strdup(((argument*)pointers_vector_get(&CURRENT_FUNCTION->arguments, 0))->name);
            i->left=(expression*)argument_name;
        } else {
            i->left=(expression*)new_empty();
        }
        i->right=copy_expression(si->right);
        request.replacement=(expression*)i;
        break;
    }
    case e_name:
    case e_member_access:
    case e_null_conditional_member_access:
    case e_indexer:
    case e_null_conditional_indexer:{
        // variable is used in some expression
        // check if it's declaration is in the same function
        // if not set it's used_in_closure to true
        // to make sure that it won't be optimised out
        char* variable=get_root_variable_name(exp);
        if(variable!=NULL) {
            VariableDeclaration* declaration=map_get(&state->declarations, variable);
            if(declaration!=NULL && declaration->owning_function!=CURRENT_FUNCTION){
                if(declaration->first_assignment->type==e_assignment){
                    ((assignment*)declaration->first_assignment)->used_in_closure=true;
                } else if(declaration->first_assignment->type==e_argument) {
                    ((argument*)declaration->first_assignment)->used_in_closure=true;
                }
            }
        }
        break;
    }
    default:;
    }
    return request;
    #undef CURRENT_FUNCTION
}

void postprocess_ast(expression** ast){
    PostprocessingState state;
    vector_init(&state.functions, sizeof(function_declaration*), 16);
    vector_push(&state.functions, ast);
    map_init(&state.declarations);
    
    visit_ast(ast, postprocess_ast_visitor, &state);
    map_deinit(&state.declarations);
}