#include "ast_postprocessing.h"

/*
Declarations map maps variable names in the current scope to their first assignment and owning function.
If a variable reference is found in a function that isn't it's owning_function then it's first_assignment used in closure.
*/
typedef struct {
    FunctionDeclaration* owning_function;
    Expression* first_assignment;
} VariableDeclaration;

typedef map_t(VariableDeclaration) DeclarationsMap;

typedef struct {
    DeclarationsMap declarations;
    vector functions;
    vector contexts;    
} PostprocessingState;

char* get_root_variable_name(Expression* expression){
    switch(expression->type){
        case e_name:
            return ((Name*)expression)->value;
        case e_member_access:
            return get_root_variable_name(((MemberAccess*)expression)->left);
        case e_null_conditional_member_access:
            return get_root_variable_name(((NullConditionalMemberAccess*)expression)->left);
        case e_indexer:
            return get_root_variable_name(((Indexer*)expression)->left);
        case e_null_conditional_indexer:
            return get_root_variable_name(((NullConditionalIndexer*)expression)->left);
        default: 
            return NULL;
    }
}

bool is_null_conditional(Expression* expression){
    return expression->type==e_null_conditional_indexer || expression->type==e_null_conditional_member_access;
}

// creates conditional that evaluates body only if null_checked is not null
Expression* create_null_conditional(Expression* null_checked, Expression* body){
    Conditional* conditional=new_conditional();
    Binary* binary=new_binary();
    binary->left=null_checked;
    binary->op=strdup("!=");
    binary->right=(Expression*)new_null_literal();
    conditional->condition=(Expression*)binary;
    conditional->ontrue=body;
    conditional->onfalse=(Expression*)new_empty();
    return (Expression*)conditional;
}

// replaces null conditional fields in expression with their not null conditional variants
void replace_null_conditional(Expression* expression) {
    switch(expression->type){
        #define EXPRESSION(struct_name, type_tag) \
            case e_##type_tag: { \
                struct_name* casted=(struct_name*)expression; \
                allow_unused_variable((void*)casted);

        // for now null conditional expressions have same memory layout as their normal variants
        #define EXPRESSION_FIELD(field_name) \
            if(casted->field_name->type==e_null_conditional_member_access) { \
                casted->field_name->type=e_member_access; \
            } \
            if(casted->field_name->type==e_null_conditional_indexer) { \
                casted->field_name->type=e_indexer; \
            }
        #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) EXPRESSION_FIELD(field_name)
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

/* 
adds 
if(<optional_argument.name>==undefined_argument) 
    <optional_argument.name>=<optional_argument.value>
to the block
*/
Expression* optional_argument_assignment(OptionalArgument* optional_argument) {
    Conditional* conditional=new_conditional();
    Binary* equals_undefined_argument=new_binary();
    Name* argument_name=new_name();
    argument_name->value=strdup(optional_argument->name);
    Name* undefined_argument_name=new_name();
    undefined_argument_name->value=strdup("undefined_argument");
    equals_undefined_argument->left=(Expression*)argument_name;
    equals_undefined_argument->right=(Expression*)undefined_argument_name;
    equals_undefined_argument->op=strdup("==");
    conditional->condition=(Expression*)equals_undefined_argument;
    Assignment* assignment=new_assignment();
    assignment->left=copy_expression((Expression*)argument_name);
    assignment->right=copy_expression(optional_argument->value);
    conditional->ontrue=(Expression*)assignment;
    conditional->onfalse=(Expression*)new_empty();
    return (Expression*)conditional;
}

void process_optional_arguments(FunctionDeclaration* function){
    Block* body;
    if(function->body->type==e_block){
        body=(Block*)function->body;
    } else {
        // function body must be a block because we need to insert new lines into it
        body=new_block();
        pointers_vector_push(&body->lines, function->body);
        function->body=(Expression*)body;
    }
    vector* arguments=&function->arguments;
    // iteration is in reverse to make the conditionals in the same order as arguments
    for(int i=vector_count(arguments)-1; i>=0; i--){
        Expression* argument=(Expression*)pointers_vector_get(arguments, i);
        if(argument->type==e_optional_argument){
            Expression* assignment=optional_argument_assignment((OptionalArgument*)argument);
            vector_insert(&body->lines, 0, &assignment);
        }
    }
}

char* extract_function_help(FunctionDeclaration* function){
    Expression* body=function->body;
    if(body->type!=e_block)
        return NULL;
    Block* body_block=(Block*)body;
    if(vector_count(&body_block->lines)<1)
        return NULL;
    Expression* first_line=pointers_vector_get(&body_block->lines, 0);
    if(first_line->type!=e_string_literal)
        return NULL;
    return strdup(((StringLiteral*)first_line)->value);
}

ASTVisitorRequest postprocess_ast_visitor(Expression* expression, void* data){
    ASTVisitorRequest request={down, NULL};
    PostprocessingState* state=data;

    // process null conditionals
    if(is_null_conditional(expression)) {
        Expression* copy=copy_expression((Expression*)expression);
        replace_null_conditional(copy);
        request.replacement=create_null_conditional(copy_expression((Expression*)((NullConditionalMemberAccess*)expression)->left), copy);
        return request;
    }
    switch(expression->type){
        #define EXPRESSION(struct_name, type_tag) \
            case e_##type_tag: { \
                struct_name* casted=(struct_name*)expression; \
                allow_unused_variable((void*)casted);

        #define EXPRESSION_FIELD(field_name) \
            if(is_null_conditional((Expression*)casted->field_name)) { \
                Expression* copy=copy_expression((Expression*)expression); \
                replace_null_conditional(copy); \
                request.replacement=create_null_conditional(copy_expression((Expression*)((NullConditionalMemberAccess*)casted->field_name)->left), copy); \
                return request; \
            }
        #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) EXPRESSION_FIELD(field_name)
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

    #define CURRENT_FUNCTION (*(FunctionDeclaration**)vector_top(&state->functions))
    switch(expression->type){
    // changing order of operations from right to left to left to right
    case e_binary: {
        Binary* b=(Binary*)expression;
        if(b->right->type==e_binary){
            Binary* b2=(Binary*)b->right;

            Binary* replacement=new_binary();
            replacement->left=copy_expression(b->left);
            replacement->right=copy_expression(b2->left);
            replacement->op=strdup(b->op);
            replacement->line_number=b->line_number;
            replacement->column_number=b->column_number;

            Binary* replacement_2=new_binary();
            replacement_2->left=(Expression*)replacement;
            replacement_2->right=copy_expression(b2->right);
            replacement_2->op=strdup(b2->op);
            replacement_2->line_number=b2->line_number;
            replacement_2->column_number=b2->column_number;

            request.replacement=(Expression*)replacement_2;
            return request;
        } else {
            // short circuit evaluation
            if(strcmp(b->op, "||")==0){
                /*
                left || right
                =>
                if(left){
                    left
                } elif(right) {
                    right
                } else {
                    null
                }
                */
                Conditional* c1=new_conditional();
                c1->condition=copy_expression(b->left);
                c1->ontrue=copy_expression(b->left);

                Conditional* c2=new_conditional();
                c2->condition=copy_expression(b->right);
                c2->ontrue=copy_expression(b->right);

                c1->onfalse=(Expression*)c2;
                c2->onfalse=(Expression*)new_null_literal();

                request.replacement=(Expression*)c1;
                return request;
            } else if(strcmp(b->op, "&&")==0){
                /*
                left && right
                =>
                if(!left){
                    left
                } elif(!right) {
                    right
                } else {
                    1
                }
                */
                Conditional* c1=new_conditional();
                Prefix* p1=new_prefix();
                p1->op=strdup("!");
                p1->right=copy_expression(b->left);
                c1->condition=(Expression*)p1;
                c1->ontrue=copy_expression(b->left);

                Conditional* c2=new_conditional();
                Prefix* p2=new_prefix();
                p2->op=strdup("!");
                p2->right=copy_expression(b->right);
                c2->condition=(Expression*)p2;
                c2->ontrue=copy_expression(b->right);

                c1->onfalse=(Expression*)c2;
                IntLiteral* one=new_int_literal();
                one->value=1;
                c2->onfalse=(Expression*)one;

                request.replacement=(Expression*)c1;
                return request;
            }
        }
        break;
    }
    case e_message: {
        /* 
            change message to function call

            messaged->message_name(arguments...)

            {
                messaged=messaged # save messaged to temporary variable to avoid executing its statement two times
                messaged.message_name(messaged, arguments...)
            }
        */
        Message* m=(Message*)expression;
        
        Name* messaged=new_name();
        messaged->value=strdup("messaged");

        // messaged=messaged
        Assignment* messaged_assignment=new_assignment();
        messaged_assignment->left=(Expression*)messaged;
        messaged_assignment->right=copy_expression(m->messaged_object);
        
        // messaged.message_name
        MemberAccess* message_function=new_member_access();
        message_function->left=copy_expression((Expression*)messaged);
        message_function->right=(Name*)copy_expression((Expression*)m->message_name);

        FunctionCall* call=new_function_call();
        call->called=(Expression*)message_function;
        // (messaged, arguments...)
        Block* call_arguments=new_block();
        pointers_vector_push(&call_arguments->lines, copy_expression((Expression*)messaged));
        for(int i=0; i<vector_count(&m->arguments->lines); i++) {
            pointers_vector_push(&call_arguments->lines, copy_expression((Expression*)pointers_vector_get(&m->arguments->lines, i)));
        }
        call->arguments=call_arguments;

        // { }
        Block* sub_scope=new_block();
        pointers_vector_push(&sub_scope->lines, messaged_assignment);
        pointers_vector_push(&sub_scope->lines, call);
        request.replacement=(Expression*)sub_scope;
        break;
    }
    case e_function_declaration: {
        // if the function is already on the stack then ast_visitor is escaping it
        if(!vector_empty(&state->functions) && *(Expression**)(vector_top(&state->functions))==expression) {
            FunctionDeclaration* f=vector_pop(&state->functions);
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
            // ast_visitor entered this function first time
            vector_push(&state->functions, (const void*)&expression);

            FunctionDeclaration* function=(FunctionDeclaration*)expression;
            if(function->optional_arguments_count>0){
                process_optional_arguments(function);
            }
            function->help=extract_function_help(function);
        }
        // intentional fallthrough
    }
    case e_table_literal: {
        // if the table literal or function is already on the stack then ast_visitor is escaping it
        if(!vector_empty(&state->contexts) && *(Expression**)(vector_top(&state->contexts))==expression) {
            vector_pop(&state->contexts);
        } else {
            // ast_visitor entered this context
            vector_push(&state->contexts, (const void*)&expression);
        }
        break;
    }
    // these three structures share their first two fields layout
    case e_variadic_argument:
    case e_optional_argument:
    case e_argument: {
        Argument* argument=(Argument*)expression;
        char* variable=argument->name;
        if(map_get(&state->declarations, variable)==NULL){
            VariableDeclaration decl={CURRENT_FUNCTION, (Expression*)argument};
            map_set(&state->declarations, variable, decl);
        }
        break;
    }
    case e_assignment: {
        Assignment* assignment=(Assignment*)expression;
        if(assignment->left->type==e_name) {
            char* variable_name=((Name*)assignment->left)->value;
            // first assignment to variable is it's declaration
            if(map_get(&state->declarations, variable_name)==NULL){
                VariableDeclaration decl={CURRENT_FUNCTION, (Expression*)assignment};
                map_set(&state->declarations, variable_name, decl);
            }
        }
        break;
    }
    case e_self_member_access: {
        // if SelfMemberAccess expression is directly inside of TableLiteral it shouldn't be transpiled into MemberAccess
        if(vector_empty(&state->contexts) || (*(Expression**)vector_top(&state->contexts))->type!=e_table_literal) {
            SelfMemberAccess* self_member_access=(SelfMemberAccess*)expression;
            MemberAccess* member_access=new_member_access();
            if(CURRENT_FUNCTION!=NULL && CURRENT_FUNCTION->type==e_function_declaration && vector_count(&CURRENT_FUNCTION->arguments)>0){
                Name* argument_name=new_name();
                argument_name->value=strdup(((Argument*)pointers_vector_get(&CURRENT_FUNCTION->arguments, 0))->name);
                member_access->left=(Expression*)argument_name;
            } else {
                member_access->left=(Expression*)new_empty();// in case it is used outside of a function
            }
            member_access->right=(Name*)copy_expression((Expression*)self_member_access->right);
            request.replacement=(Expression*)member_access;
        }
        break;
    }
    case e_self_indexer: {
        if(vector_empty(&state->contexts) || (*(Expression**)vector_top(&state->contexts))->type!=e_table_literal) {
            SelfIndexer* self_indexer=(SelfIndexer*)expression;
            Indexer* indexer=new_indexer();
            if(CURRENT_FUNCTION!=NULL && CURRENT_FUNCTION->type==e_function_declaration && vector_count(&CURRENT_FUNCTION->arguments)>0){
                Name* argument_name=new_name();
                argument_name->value=strdup(((Argument*)pointers_vector_get(&CURRENT_FUNCTION->arguments, 0))->name);
                indexer->left=(Expression*)argument_name;
            } else {
                indexer->left=(Expression*)new_empty();
            }
            indexer->right=copy_expression(self_indexer->right);
            request.replacement=(Expression*)indexer;
        }
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
        char* variable=get_root_variable_name(expression);
        if(variable!=NULL) {
            VariableDeclaration* declaration=map_get(&state->declarations, variable);
            if(declaration!=NULL && declaration->owning_function!=CURRENT_FUNCTION){
                if(declaration->first_assignment->type==e_assignment){
                    ((Assignment*)declaration->first_assignment)->used_in_closure=true;
                } else if(declaration->first_assignment->type==e_variadic_argument
                       || declaration->first_assignment->type==e_optional_argument
                       || declaration->first_assignment->type==e_argument) {
                    // these three structures share their first two fields layout
                    ((Argument*)declaration->first_assignment)->used_in_closure=true;
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

void postprocess_ast(Expression** ast){
    PostprocessingState state;
    vector_init(&state.functions, sizeof(FunctionDeclaration*), 16);
    vector_push(&state.functions, ast);
    vector_init(&state.contexts, sizeof(Expression*), 16);
    map_init(&state.declarations);
    
    visit_ast(ast, postprocess_ast_visitor, &state);

    vector_deinit(&state.functions);
    vector_deinit(&state.contexts);
    map_deinit(&state.declarations);
}