#include "macros.h"

typedef expression* (*ExpressionInitializer)(void);

// TODO: Remove these globals
static map_t(Object) expression_fields_map;
typedef map_t(ExpressionInitializer) ExpressionInitializersMap;
static ExpressionInitializersMap expression_initializers_map;
static bool expression_initializers_map_initialized=false;
static Object expression_fields_array[EXPRESSION_TYPES_COUNT];

Object expression_struct_descriptor_get(Executor* E, Object* arguments, int argumets_count);
Object expression_fields(Executor* E, expression* exp);

Object expression_destroy_recursively(Executor* E, Table* field, expression* expression_pointer){
    //printf("<%s>", stringify_expression(expression_pointer, 0));
    Object fields=get_table(field, to_string("fields"));
    REQUIRE_TYPE(fields, t_table);
    TableIterator it=start_iteration(fields.tp);
    for(IterationResult i=table_next(&it); !i.finished; i=table_next(&it)) {
        REQUIRE_TYPE(i.value, t_table);
        Object type_field=get_table(i.value.tp, to_string("type"));
        REQUIRE_TYPE(type_field, t_number);
        if(type_field.value==n_pointer){
            Object pointed=get_table(i.value.tp, to_string("pointed"));
            REQUIRE_TYPE(i.value, t_table);
            Object has_ownership=get_table(pointed.tp, to_string("has_ownership"));
            if(is_falsy(has_ownership)){
                Object offset=get_table(i.value.tp, to_string("offset"));
                REQUIRE_TYPE(offset, t_number);
                expression** expression_position=(expression**)((int)expression_pointer+(int)offset.value);
                //expression** expression_position=(expression**)position.p;
                expression_destroy_recursively(E, pointed.tp, *expression_position);
            }
            destroy_unreferenced(E, &has_ownership);
        }
        /* 
        if i.value is a expression pointer
        get it's pointed table and check if it contains destructor
        if not you should call this function on it
        but id doesn't contain field position so you need to pass it by dereferencing pointer
        */
    }
    delete_expression_keep_children(expression_pointer);
    return null_const;
}

Object expression_destroy(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table);
    return expression_destroy_recursively(E, self.tp, (expression*)struct_descriptor_get_pointer(E, self.tp));
}

void postproccess_expression_descriptor(Executor* E, Table* descriptor){
    set_table(E, descriptor, to_string("is_expression"), to_number(1));
    set_table(E, descriptor, to_string("destroy"), to_function(E, expression_destroy, NULL, 1));
    set_table(E, descriptor, to_string("has_ownership"), to_number(1));
    if(get_table(descriptor, to_string("replaced_get")).type==t_null){
        set_table(E, descriptor, to_string("replaced_get"), get_table(descriptor, to_string("get")));
        set_table(E, descriptor, to_string("get"), to_function(E, expression_struct_descriptor_get, NULL, 2));
    }
}

Object expression_copy(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table);
    expression* copy=copy_expression(struct_descriptor_get_pointer(E, self.tp));
    Object sd=new_struct_descriptor(E, (void*)copy, expression_fields(E, copy));
    postproccess_expression_descriptor(E, sd.tp);
    return sd;
}

/* 
get override function
gets a field from expression struct descriptor and then downcasts it to proper type depedning on expression type field
by changing it's fields table to one gotten from expression_fields
*/
Object expression_struct_descriptor_get(Executor* E, Object* arguments, int argumets_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table);
    Object replaced_get=get_table(self.tp, to_string("replaced_get"));
    Object get_result=call(E, replaced_get, arguments, argumets_count);
    if(is_struct_descriptor(E, get_result)){
        // downcast the value
        set_table(E, get_result.tp, to_string("fields"), expression_fields(E, (expression*)struct_descriptor_get_pointer(E, get_result.tp)));
        postproccess_expression_descriptor(E, get_result.tp);
    }
    return get_result;
}

Object expression_fields(Executor* E, expression* exp) {
    if(exp==NULL){
        return null_const;
    } else if(expression_fields_array[exp->type].type!=t_null){
        return expression_fields_array[exp->type];
    }
    switch(exp->type){
        #define EXPRESSION(etype) \
            case e_##etype: {\
            etype* casted=new_##etype();\
            allow_unused_variable(&casted);\
            Object fields;\
            table_init(E, &fields);\
            map_set(&expression_fields_map, #etype, fields);\
            expression_fields_array[e_##etype]=fields;\
            reference(&fields);\
            set(E, fields, to_string("expression_type"), to_field(E, OFFSET(*casted, type), n_int));
        #define FIELD(field_name, field_descriptor) \
            { Object field_temp=field_descriptor; \
            set(E, fields, to_string(#field_name), field_temp); }
        #define SPECIFIED_EXPRESSION_FIELD(type, field_name)\
            casted->field_name=new_##type();\
            FIELD(field_name, to_struct_pointer_field(E, OFFSET(*casted, field_name), expression_fields(E, (expression*)casted->field_name)));
        #define EXPRESSION_FIELD(field_name)\
            casted->field_name=new_expression();\
            FIELD(field_name, to_struct_pointer_field(E, OFFSET(*casted, field_name), expression_fields(E, casted->field_name)));
        #define BOOL_FIELD(field_name)                       set(E, fields, to_string(#field_name), to_field(E, OFFSET(*casted, field_name), n_int));
        #define FLOAT_FIELD(field_name)                      set(E, fields, to_string(#field_name), to_field(E, OFFSET(*casted, field_name), n_float));
        #define INT_FIELD(field_name)                        set(E, fields, to_string(#field_name), to_field(E, OFFSET(*casted, field_name), n_int));
        #define STRING_FIELD(field_name)                     set(E, fields, to_string(#field_name), to_field(E, OFFSET(*casted, field_name), n_string));
        #define VECTOR_FIELD(field_name)                     // TODO
        #define END delete_expression((expression*)casted); return fields; }
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

Object make_expression(Executor* E, Object* arguments, int arguments_count){
    Object expression_name=arguments[0];
    REQUIRE_TYPE(expression_name, t_string);
    ExpressionInitializer* initializer=map_get(&expression_initializers_map, expression_name.text);
    if(initializer!=NULL){
        expression* exp=(*initializer)();
        exp->line_number=E->line;
        exp->column_number=0;
        Object sd=new_struct_descriptor(E, (void*)exp, expression_fields(E, exp));
        postproccess_expression_descriptor(E, sd.tp);
        return sd;
    } else {
        return null_const;
    }
}

void register_ast_types(Executor* E, Object scope){
    if(expression_initializers_map_initialized){
        return;
    }
    map_init(&expression_initializers_map);
    #define EXPRESSION(type) map_set(&expression_initializers_map, #type, (ExpressionInitializer)&new_##type);
    #define SPECIFIED_EXPRESSION_FIELD(type, field_name)
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

    expression_initializers_map_initialized=true;
    set_function(E, scope, "copy_expression", 1, false, expression_copy);
    set_function(E, scope, "new_expression", 1, false, make_expression);
}

Object wrap_expression(Executor* E, expression* exp){
    Object wrapped=new_struct_descriptor(E, (void*)exp, expression_fields(E, exp));
    REQUIRE_TYPE(wrapped, t_table)
    postproccess_expression_descriptor(E, wrapped.tp);
    return wrapped;
}

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
    stream_push(&s, "\0", 1);
    return stream_get_data(&s);
}

ast_visitor_request remove_nulls_from_expression_visitor(expression* exp, void* data){
    if(exp==NULL) {
        ast_visitor_request request={down, (expression*)new_empty()};
        return request;
    }
    switch(exp->type){
        #define EXPRESSION(type) \
            case e_##type: { \
            type* casted=(type*)exp; \
            allow_unused_variable(casted);
        
        #define SPECIFIED_EXPRESSION_FIELD(type, field_name)
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
    ast_visitor_request request={down, NULL};
    return request;
}

void remove_nulls_from_expression(expression* exp){
    visit_ast(exp, remove_nulls_from_expression_visitor, NULL);
}

expression* to_literal(Object o);
expression* to_expression(Executor* E, Object o){
    if(is_struct_descriptor(E, o)) {
        Object is_expression=get_table(o.tp, to_string("is_expression"));
        if(!is_falsy(is_expression)) {
            dereference(E, &is_expression);
            expression* exp=(expression*)struct_descriptor_get_pointer(E, o.tp);
            set_table(E, o.tp, to_string("destroy"), null_const);// make sure that dereferencing the struct descriptor won't destroy the expression
            if(exp!=NULL){
                remove_nulls_from_expression(exp);
            }
            return exp;
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

void proccess_macro_declaration(macro_declaration* md, MacroVisitorState* state){
    Object scope;
    table_init(state->executor, &scope);
    register_builtins(state->executor, scope);
    register_ast_types(state->executor, scope);
    char* key=path_to_string(md->left->pth);
    map_set(&state->macro_definitions, key, evaluate(state->executor, md->right, scope, false));
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
                    arguments[j]=wrap_expression(state->executor, copy_expression(vector_get(&b->lines, i+1+j)));\
                    reference(&arguments[j]);
                }
                Object evaluation_result=evaluate_macro(state->executor, macro_value, arguments, expected_arguments);
                expression* converted=to_expression(state->executor, evaluation_result);
                if(converted!=NULL) {
                    // replace macro with it's evaluation result
                    vector_set(&b->lines, i, converted);
                    delete_expression((expression*)m);
                    // remove arguments
                    for(int j=0; j<expected_arguments; j++){
                        //delete_expression(vector_get(&b->lines, i+1));
                        dereference(state->executor, &arguments[j]);
                        vector_delete(&b->lines, i+1);
                    }
                    free(key);
                    dereference(state->executor, &evaluation_result);
                    ast_visitor_request request={down};
                    return request;
                } else {
                    free(key);
                    dereference(state->executor, &evaluation_result);
                    USING_STRING(stringify(state->executor, evaluation_result),
                        THROW_ERROR(AST_ERROR, "Can't put back the result of evaluating macro \"%s\", the result is: \n%s", key, str))
                }
            }
        }
    } else if(exp->type==e_macro){
        macro* m=(macro*)exp;
        char* key=path_to_string(m->pth);
        Object* map_get_result=map_get(&state->macro_definitions, key);
        if(map_get_result==NULL){
            THROW_ERROR(AST_ERROR, "Unknown macro %s.", key)
        }
        Object evaluation_result=evaluate_macro(state->executor, *map_get_result, NULL, 0);
        expression* converted=to_expression(state->executor, evaluation_result);
        if(converted!=NULL) {
            ast_visitor_request request={next, converted};
            free(key);
            return request;
        } else {
            USING_STRING(stringify(state->executor, evaluation_result),
                THROW_ERROR(AST_ERROR, "Can't put back the result of evaluating macro \"%s\", the result is \n%s", key, str))
            free(key);
        }
    }
    ast_visitor_request request={down, NULL};
    return request;
}

void execute_macros(Executor* E, expression* ast){
    MacroVisitorState state;
    state.executor=E;
    map_init(&state.macro_definitions);
    visit_ast(ast, macro_visitor, &state);
    map_deinit(&state.macro_definitions);
}