#include "macros.h"

typedef Expression* (*ExpressionInitializer)(void);

// TODO: Remove these globals
static map_t(Object) expression_fields_map;
static Object expression_fields_array[EXPRESSION_TYPES_COUNT];

Object expression_descriptor_get(Executor* E, Object scope, Object* arguments, int arguments_count);
Object expression_fields(Executor* E, Expression* expression);

void downcast_expression_descriptor(Executor* E, Table* sd){
    Object fields=table_get(E, sd, to_string("fields"));
    if(fields.type!=t_table){
        return;
    }
    Object fields_expression_type=table_get(E, fields.tp, to_string("fields_expression_type"));
    if(fields_expression_type.type!=t_int){
        return;
    }
    Expression* expression=(Expression*)struct_descriptor_get_pointer(E, sd);
    if(expression->type!=(int)fields_expression_type.int_value){
        table_set(E, sd, to_string("fields"), copy(E, expression_fields(E, expression)));
    }
}

Object expression_descriptor_destroy_recursively(Executor* E, Table* sd, Expression* expression_pointer){
    downcast_expression_descriptor(E, sd);
    //printf("<%s>", stringify_expression(expression_pointer, 0));
    Object fields=table_get(E, sd, to_string("fields"));
    REQUIRE_TYPE(fields, t_table);
    TableIterator it=table_get_iterator(fields.tp);
    for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
        if(i.value.type!=t_table){
            continue;
        }
        Object type_field=table_get(E, i.value.tp, to_string("type"));
        REQUIRE_TYPE(type_field, t_int);
        if(type_field.int_value==n_pointer){
            Object pointed=table_get(E, i.value.tp, to_string("pointed"));
            REQUIRE_TYPE(i.value, t_table);
            Object has_ownership=table_get(E, pointed.tp, to_string("has_ownership"));
            if(is_truthy(has_ownership)){
                Object offset=table_get(E, i.value.tp, to_string("offset"));
                REQUIRE_TYPE(offset, t_int);
                Expression** expression_position=(Expression**)((int)expression_pointer+(int)offset.int_value);
                //Expression** expression_position=(Expression**)position.p;
                expression_descriptor_destroy_recursively(E, pointed.tp, *expression_position);
            }
            destroy_unreferenced(E, &has_ownership);
        }
        /* 
        if i.value is a expression pointer
        get it's pointed table and check if it contains destructor
        if not you should call this function on it
        but id doesn't contain sd position so you need to pass it by dereferencing pointer
        */
    }
    delete_expression_keep_children(expression_pointer);
    return null_const;
}

Object expression_descriptor_destroy(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table);
    return expression_descriptor_destroy_recursively(E, self.tp, (Expression*)struct_descriptor_get_pointer(E, self.tp));
}

void postprocess_expression_descriptor(Executor* E, Table* descriptor){
    table_set(E, descriptor, to_string("is_expression"), to_int(1));
    table_set(E, descriptor, OVERRIDE(E, destroy), to_native_function(E, expression_descriptor_destroy, NULL, 1, false));
    table_set(E, descriptor, to_string("has_ownership"), to_int(1));
    if(table_get(E, descriptor, to_string("replaced_get")).type==t_null){
        table_set(E, descriptor, to_string("replaced_get"), table_get(E, descriptor, to_string("get")));
        table_set(E, descriptor, OVERRIDE(E, get), to_native_function(E, expression_descriptor_get, NULL, 2, false));
    }
}

Object expression_descriptor_copy(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table);
    Expression* copy=copy_expression(struct_descriptor_get_pointer(E, self.tp));
    Object sd=new_struct_descriptor(E, (void*)copy, expression_fields(E, copy));
    postprocess_expression_descriptor(E, sd.tp);
    return sd;
}

/* 
get override function
gets a field from expression struct descriptor and then downcasts it to proper type depending on expression type field
by changing it's fields table to one gotten from expression_fields
*/
Object expression_descriptor_get(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table);
    Object replaced_get=table_get(E, self.tp, to_string("replaced_get"));
    Object get_result=call(E, replaced_get, arguments, arguments_count);
    if(is_struct_descriptor(E, get_result)){
        downcast_expression_descriptor(E, get_result.tp);
        postprocess_expression_descriptor(E, get_result.tp);
    }
    return get_result;
}

Object expression_fields(Executor* E, Expression* expression) {
    if(expression==NULL){
        return null_const;
    } else if(expression_fields_array[expression->type].type!=t_null){
        return expression_fields_array[expression->type];
    }
    switch(expression->type){
        #define EXPRESSION(struct_name, type_tag) \
        case e_##type_tag: { \
            struct_name* casted=new_##type_tag(); \
            allow_unused_variable(&casted); \
            Object fields; \
            table_init(E, &fields);\
            map_set(&expression_fields_map, #type_tag, fields); \
            expression_fields_array[e_##type_tag]=fields; \
            reference(&fields); \
            set(E, fields, to_string("expression_type"), to_field(E, OFFSET(*casted, type), n_int)); \
            set(E, fields, to_string("fields_expression_type"), to_int(expression->type));
        #define FIELD(field_name, field_descriptor) \
            { Object field_temp=field_descriptor; \
            set(E, fields, to_string(#field_name), field_temp); }
        #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) \
            casted->field_name=new_##type_tag(); \
            FIELD(field_name, to_struct_pointer_field(E, OFFSET(*casted, field_name), expression_fields(E, (Expression*)casted->field_name)));
        #define EXPRESSION_FIELD(field_name) \
            casted->field_name=new_expression(); \
            FIELD(field_name, to_struct_pointer_field(E, OFFSET(*casted, field_name), expression_fields(E, casted->field_name)));
        #define BOOL_FIELD(field_name)                       set(E, fields, to_string(#field_name), to_field(E, OFFSET(*casted, field_name), n_int));
        #define FLOAT_FIELD(field_name)                      set(E, fields, to_string(#field_name), to_field(E, OFFSET(*casted, field_name), n_float));
        #define INT_FIELD(field_name)                        set(E, fields, to_string(#field_name), to_field(E, OFFSET(*casted, field_name), n_int));
        #define STRING_FIELD(field_name)                     set(E, fields, to_string(#field_name), to_field(E, OFFSET(*casted, field_name), n_string));
        #define VECTOR_FIELD(field_name)                     // TODO
        #define END \
            delete_expression((Expression*)casted); \
            return fields; }
        default: return null_const;

        AST_EXPRESSIONS

        #undef EXPRESSION
        #undef FIELD
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

Object new_expression_descriptor(Executor* E, Object scope, Object* arguments, int arguments_count){
    REQUIRE_TYPE(scope, t_pointer)
    ExpressionInitializer initializer=(ExpressionInitializer)scope.p;
    if(initializer!=NULL){
        Expression* expression=initializer();
        expression->line_number=E->line;
        expression->column_number=0;
        Object sd=new_struct_descriptor(E, (void*)expression, expression_fields(E, expression));
        postprocess_expression_descriptor(E, sd.tp);
        return sd;
    } else {
        return null_const;
    }
}

void register_ast_types(Executor* E, Object scope){
    #define EXPRESSION(struct_name, type_tag) \
    {   Object function=to_native_function(E, new_expression_descriptor, NULL, 0, false); \
        function.fp->enclosing_scope=to_pointer(new_##type_tag); \
        set(E, scope, to_string("new_"#type_tag), function); }
    #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name)
    #define EXPRESSION_FIELD(field_name)
    #define BOOL_FIELD(field_name)                     
    #define FLOAT_FIELD(field_name)                    
    #define INT_FIELD(field_name)                      
    #define STRING_FIELD(field_name)                   
    #define VECTOR_FIELD(field_name) 
    #define END

    AST_EXPRESSIONS

    #undef EXPRESSION
    #undef SPECIFIED_EXPRESSION_FIELD
    #undef EXPRESSION_FIELD               
    #undef BOOL_FIELD                   
    #undef STRING_FIELD
    #undef VECTOR_FIELD
    #undef FLOAT_FIELD
    #undef INT_FIELD
    #undef END

    set_function(E, scope, "copy_expression", 1, false, expression_descriptor_copy);
}

Object wrap_expression(Executor* E, Expression* expression){
    Object wrapped=new_struct_descriptor(E, (void*)expression, expression_fields(E, expression));
    REQUIRE_TYPE(wrapped, t_table)
    postprocess_expression_descriptor(E, wrapped.tp);
    return wrapped;
}

typedef struct {
    Executor* executor;
    map_t(Object) macro_definitions;
} MacroVisitorState;

ASTVisitorRequest remove_nulls_from_expression_visitor(Expression* expression, void* data){
    if(expression==NULL) {
        ASTVisitorRequest request={down, (Expression*)new_empty()};
        return request;
    }
    switch(expression->type){
        #define EXPRESSION(struct_name, type_tag) \
            case e_##type_tag: { \
            struct_name* casted=(struct_name*)expression; \
            allow_unused_variable(casted);
        
        #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name)
        #define EXPRESSION_FIELD(field_name)
        #define BOOL_FIELD(field_name)
        #define FLOAT_FIELD(field_name)
        #define INT_FIELD(field_name)
        #define STRING_FIELD(field_name) if(casted->field_name==NULL) casted->field_name=strdup("");
        #define VECTOR_FIELD(field_name)
        #define END break; }

        AST_EXPRESSIONS

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
    ASTVisitorRequest request={down, NULL};
    return request;
}

void remove_nulls_from_expression(Expression** expression){
    visit_ast(expression, remove_nulls_from_expression_visitor, NULL);
}

Expression* to_literal(Object o);
Expression* to_expression(Executor* E, Object o){
    if(is_struct_descriptor(E, o)) {
        Object is_expression=table_get(E, o.tp, to_string("is_expression"));
        if(is_truthy(is_expression)) {
            dereference(E, &is_expression);
            Expression* expression=(Expression*)struct_descriptor_get_pointer(E, o.tp);
            table_set(E, o.tp, OVERRIDE(E, destroy), null_const);// make sure that dereferencing the struct descriptor won't destroy the expression
            if(expression!=NULL){
                remove_nulls_from_expression(&expression);
            }
            return expression;
        }
        dereference(E, &is_expression);
    }
    return to_literal(o);
}

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
    Executor* E= state->executor;
    Object scope;
    table_init(E, &scope);
    if(E->options.include_builtins){
        inherit_scope(E, scope, builtins_table(E));
    }
    register_ast_types(E, scope);
    map_set(&state->macro_definitions, md->left->identifier->value, evaluate(E, md->right, scope, "macro", false));
}

ASTVisitorRequest macro_visitor(Expression* expression, void* data){
    MacroVisitorState* state=(MacroVisitorState*)data;
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
                Macro* m=(Macro*)line;
                char* key=m->identifier->value;

                Object* map_get_result=map_get(&state->macro_definitions, key);
                if(map_get_result==NULL){
                    THROW_ERROR(AST_ERROR, "Unknown macro %s.", key)
                }
                Object macro_value=*map_get_result;
                
                int expected_arguments=macro_arguments_count(macro_value);
                if(expected_arguments>vector_count(&b->lines)-i-1){
                    USING_STRING(stringify_expression(expression, 0),
                        THROW_ERROR(AST_ERROR, "There is not enough lines in block: %s for %s macro to work.", str, key))
                }
                Object* arguments=malloc(sizeof(Object)*expected_arguments);
                for(int j=0; j<expected_arguments; j++){
                    arguments[j]=wrap_expression(state->executor, copy_expression(pointers_vector_get(&b->lines, i+1+j)));\
                    reference(&arguments[j]);
                }
                Object evaluation_result=evaluate_macro(state->executor, macro_value, arguments, expected_arguments);
                Expression* converted=to_expression(state->executor, evaluation_result);
                if(converted!=NULL) {
                    // replace macro with it's evaluation result
                    pointers_vector_set(&b->lines, i, converted);
                    delete_expression((Expression*)m);
                    // remove arguments
                    for(int j=0; j<expected_arguments; j++){
                        //delete_expression(pointers_vector_get(&b->lines, i+1));
                        dereference(state->executor, &arguments[j]);
                        vector_delete(&b->lines, i+1);
                    }
                    dereference(state->executor, &evaluation_result);
                    ASTVisitorRequest request={down};
                    return request;
                } else {
                    dereference(state->executor, &evaluation_result);
                    USING_STRING(stringify(state->executor, evaluation_result),
                        THROW_ERROR(AST_ERROR, "Can't put back the result of evaluating macro \"%s\", the result is: \n%s", key, str))
                }
            }
        }
    } else if(expression->type==e_macro){
        Macro* m=(Macro*)expression;
        char* key=m->identifier->value;
        Object* map_get_result=map_get(&state->macro_definitions, key);
        if(map_get_result==NULL){
            THROW_ERROR(AST_ERROR, "Unknown macro %s.", key)
        }
        Object evaluation_result=evaluate_macro(state->executor, *map_get_result, NULL, 0);
        Expression* converted=to_expression(state->executor, evaluation_result);
        if(converted!=NULL) {
            ASTVisitorRequest request={next, converted};
            free(key);
            return request;
        } else {
            USING_STRING(stringify(state->executor, evaluation_result),
                THROW_ERROR(AST_ERROR, "Can't put back the result of evaluating macro \"%s\", the result is \n%s", key, str))
            free(key);
        }
    }
    ASTVisitorRequest request={down, NULL};
    return request;
}

void execute_macros(Executor* E, Expression** ast){
    MacroVisitorState state;
    state.executor=E;
    map_init(&state.macro_definitions);
    visit_ast(ast, macro_visitor, &state);
    map_deinit(&state.macro_definitions);
}