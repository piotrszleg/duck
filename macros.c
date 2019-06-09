#include "macros.h"

map_t(Object) expression_classes_map;
Object expression_classes_array[EXPRESSION_TYPES_COUNT];

Object get_expression_downcasted(Executor* E, Object* arguments, int argumets_count);
Object expression_class(Executor* E, expression* exp);

void postproccess_expression_descriptor(Executor* E, Table* descriptor){
    set_table(E, descriptor, to_string("is_expression"), to_number(1));
    if(get_table(descriptor, to_string("replaced_get")).type==t_null){
        set_table(E, descriptor, to_string("replaced_get"), get_table(descriptor, to_string("get")));
        set_table(E, descriptor, to_string("get"), to_function(E, get_expression_downcasted, NULL, 2));
    }
}

Object get_expression_downcasted(Executor* E, Object* arguments, int argumets_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table);
    Object replaced_get=get_table(self.tp, to_string("replaced_get"));
    Object get_result=call(E, replaced_get, arguments, argumets_count);
    if(get_result.type==t_table && is_falsy(get_table(get_result.tp, to_string("error")))){
        if(get_table(get_result.tp, to_string("type")).value==n_pointer){
            Object pointed_field=get_table(get_result.tp, to_string("pointed"));
            expression** pointed=(expression**)get_table(get_result.tp, to_string("position")).p;
            REQUIRE_TYPE(pointed_field, t_table)
            set_table(E, pointed_field.tp, to_string("class"), expression_class(E,  *pointed));
        } else{
            Object type=call(E, replaced_get, (Object[]){get_result, to_string("expression_type")}, 2);
            REQUIRE_TYPE(type, t_number)
            set_table(E, get_result.tp, to_string("class"), expression_classes_array[(int)type.value]);
        }
        postproccess_expression_descriptor(E, get_result.tp);
    }
    return get_result;
}

Object expression_class(Executor* E, expression* exp) {
    if(exp==NULL){
        return null_const;
    } else if(expression_classes_array[exp->type].type!=t_null){
        return expression_classes_array[exp->type];
    }
    switch(exp->type){
        #define EXPRESSION(etype) \
            case e_##etype: {\
            etype* casted=new_##etype();\
            allow_unused_variable(&casted);\
            Object class;\
            table_init(E, &class);\
            map_set(&expression_classes_map, #etype, class);\
            expression_classes_array[e_##etype]=class;\
            set(E, class, to_string("expression_type"), to_field(E, OFFSET(*casted, type), n_int));
        #define FIELD(field_name, field_descriptor) \
            { Object field_temp=field_descriptor; \
            set(E, class, to_string(#field_name), field_temp); }
        #define SPECIFIED_EXPRESSION_FIELD(type, field_name)\
            casted->field_name=new_##type();\
            FIELD(field_name, to_struct_pointer_field(E, OFFSET(*casted, field_name), expression_class(E, (expression*)casted->field_name)));
        #define EXPRESSION_FIELD(field_name)\
            casted->field_name=new_expression();\
            FIELD(field_name, to_struct_pointer_field(E, OFFSET(*casted, field_name), expression_class(E, casted->field_name)));
        #define BOOL_FIELD(field_name)                       set(E, class, to_string(#field_name), to_field(E, OFFSET(*casted, field_name), n_int));
        #define FLOAT_FIELD(field_name)                      set(E, class, to_string(#field_name), to_field(E, OFFSET(*casted, field_name), n_float));
        #define INT_FIELD(field_name)                        set(E, class, to_string(#field_name), to_field(E, OFFSET(*casted, field_name), n_int));
        #define STRING_FIELD(field_name)                     set(E, class, to_string(#field_name), to_field(E, OFFSET(*casted, field_name), n_string));
        #define VECTOR_FIELD(field_name)                     // TODO
        #define END delete_expression((expression*)casted); return class; }
        default: return null_const;

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
}

Object wrap_expression(Executor* E, expression* exp){
    Object wrapped=new_struct_descriptor(E, (void*)exp, expression_class(E, exp));
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

expression* to_literal(Object o);

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
                    arguments[j]=wrap_expression(state->executor, copy_expression(vector_get(&b->lines, i+1+j)));
                }
                Object evaluation_result=evaluate_macro(state->executor, macro_value, arguments, expected_arguments);
                expression* converted=to_literal(evaluation_result);
                if(converted!=NULL) {
                    // replace macro with it's evaluation result
                    vector_set(&b->lines, i, converted);
                    delete_expression((expression*)m);
                    // remove arguments
                    for(int j=0; j<expected_arguments; j++){
                        delete_expression(vector_get(&b->lines, i+1));
                        vector_delete(&b->lines, i+1);
                    }
                    free(key);
                    ast_visitor_request request={down};
                    return request;
                } else {
                    USING_STRING(stringify(state->executor, evaluation_result),
                        THROW_ERROR(AST_ERROR, "Can't put back the result of evaluating macro \"%s\", the result is: \n%s", key, str))
                    free(key);
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
        expression* converted=to_literal(evaluation_result);
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