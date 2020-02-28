#include "expression_object.h"

Expression* object_to_literal(Executor* E, Object o){
    switch(o.type){
        case t_string:
        {
            StringLiteral* l=new_string_literal();
            l->value=strdup(o.text);
            return (Expression*)l;
        }
        case t_int:
        {
            IntLiteral* l=new_int_literal();
            l->value=o.int_value;
            return (Expression*)l;
        }
        case t_float:
        {
            FloatLiteral* l=new_float_literal();
            l->value=o.float_value;
            return (Expression*)l;
        }
        case t_null:
            return (Expression*)new_empty();
        case t_table:
            handle_if_error(E, o);
            TableLiteral* table_literal=new_table_literal();
            TableIterator it=table_get_iterator(o.tp);
            for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
                if(i.key.type==t_int && i.key.int_value==0){
                    pointers_vector_push(&table_literal->lines, object_to_literal(E, i.value));
                } else {
                    Assignment* assignment=new_assignment();
                    pointers_vector_push(&table_literal->lines, assignment);
                    #define CHECK_EXPRESSION(expression) \
                        if(expression==NULL){ \
                            delete_expression((Expression*)table_literal); \
                            return NULL; \
                        }
                
                    if(i.key.type==t_string && is_valid_name(i.key.text)){
                        Name* name=new_name();
                        name->value=strdup(i.key.text);
                        assignment->left=(Expression*)name;
                    } else {
                        SelfIndexer* indexer=new_self_indexer();
                        indexer->right=object_to_literal(E, i.key);
                        CHECK_EXPRESSION(indexer->right)
                        assignment->left=(Expression*)indexer;
                    }
                    assignment->right=object_to_literal(E, i.value);
                    CHECK_EXPRESSION(assignment->right)
                }
            }
            return (Expression*)table_literal;
        default:
            return NULL;
    }
}

Object expression_to_object(Executor* E, Expression* expression){
    if(expression->type==e_int_literal){
        return to_int(((IntLiteral*)expression)->value);
    } else if(expression->type==e_float_literal){
        return to_float(((FloatLiteral*)expression)->value);
    }else if(expression->type==e_string_literal){
        return to_string(strdup(((StringLiteral*)expression)->value));
    }
    switch(expression->type){
        #define EXPRESSION(struct_name, type_tag) \
        case e_##type_tag: { \
            struct_name* casted=(struct_name*)expression; \
            allow_unused_variable(&casted); \
            Object result; \
            table_init(E, &result);\
            table_set(E, result.tp, to_int(0), to_string(#type_tag));
        #define EXPRESSION_FIELD(field_name) \
            table_set(E, result.tp, to_string(#field_name), expression_to_object(E, (Expression*)casted->field_name)); 
        #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) EXPRESSION_FIELD(field_name)
        #define BOOL_FIELD(field_name)   table_set(E, result.tp, to_string(#field_name), to_int(casted->field_name));
        #define FLOAT_FIELD(field_name)  table_set(E, result.tp, to_string(#field_name), to_float(casted->field_name));
        #define INT_FIELD(field_name)    table_set(E, result.tp, to_string(#field_name), to_int(casted->field_name));
        #define STRING_FIELD(field_name) table_set(E, result.tp, to_string(#field_name), to_string(casted->field_name));
        #define VECTOR_FIELD(field_name) \
        {   Object converted_vector; \
            table_init(E, &converted_vector); \
            for(int i=0; i<vector_count(&casted->field_name); i++) { \
                table_set(E, converted_vector.tp, to_int(i), \
                    expression_to_object(E, pointers_vector_get(&casted->field_name, i))); \
            } \
            table_set(E, result.tp, to_string(#field_name), converted_vector); \
            dereference(E, &converted_vector);   }
        #define END                      return result; }
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

Expression* object_to_expression(Executor* E, Object o);

Expression* table_to_expression(Executor* E, Table* table){
    Object type=table_get(E, table, to_int(0));
    if(type.type!=t_string){
        return NULL;
    }
    #define EXPRESSION(struct_name, type_tag) \
    if(strcmp(type.text, #type_tag)==0) { \
        struct_name* result=new_##type_tag();
    #define EXPRESSION_FIELD(field_name) \
        {   Object field=table_get(E, table, to_string(#field_name)); \
            result->field_name=object_to_expression(E, field);   } 
    #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) \
        {   Object field=table_get(E, table, to_string(#field_name)); \
            result->field_name=(struct_name*)object_to_expression(E, field); }
    #define TYPED_FIELD(field_name, object_type, value) \
        {   Object field=table_get(E, table, to_string(#field_name)); \
            if(field.type==object_type) result->field_name=value; }
    #define BOOL_FIELD(field_name)   TYPED_FIELD(field_name, t_int, field.int_value) 
    #define FLOAT_FIELD(field_name)  TYPED_FIELD(field_name, t_float, field.float_value)
    #define INT_FIELD(field_name)    TYPED_FIELD(field_name, t_int, field.int_value)
    #define STRING_FIELD(field_name) TYPED_FIELD(field_name, t_string, strdup(field.text))
    #define VECTOR_FIELD(field_name) \
    {   \
        Object field=table_get(E, table, to_string(#field_name)); \
        if(field.type==t_table){ \
            for(int i=0; ; i++){ \
                Object at_index=table_get(E, field.tp, to_int(i)); \
                if(at_index.type==t_null){ \
                    break; \
                } else { \
                    pointers_vector_push(&result->field_name, object_to_expression(E, at_index)); \
                } \
            } \
        } \
    }
    #define END \
        remove_nulls_from_expression((Expression**)&result); \
        return (Expression*)result; }
    
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

    return NULL;
}

Expression* object_to_expression(Executor* E, Object o){
    if(o.type==t_table){
        return table_to_expression(E, o.tp);
    } else {
        return object_to_literal(E, o);
    }
}