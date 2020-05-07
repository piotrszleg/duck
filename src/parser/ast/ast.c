#include "ast.h"

#ifdef COUNT_AST_ALLOCATIONS
static int ast_allocations_count=0;
#define AST_ALLOCATION   ast_allocations_count++;
#define AST_DEALLOCATION ast_allocations_count--;
// used for checking if all expressions were properly deleted
bool ast_allocations_zero(){
    return ast_allocations_count==0;
}
#else
#define AST_ALLOCATION
#define AST_DEALLOCATION
#endif

#define EXPRESSION(struct_name, type_tag) \
struct_name* new_ ## type_tag(){  \
    struct_name* instance=malloc(sizeof(struct_name)); \
    CHECK_ALLOCATION(instance); \
    AST_ALLOCATION \
	instance->type=e_ ## type_tag; \

#define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) instance->field_name=(struct_name*)NULL;
#define EXPRESSION_FIELD(field_name) instance->field_name=NULL;
#define BOOL_FIELD(field_name) instance->field_name=false;
#define STRING_FIELD(field_name) instance->field_name=NULL;
#define INT_FIELD(field_name) instance->field_name=0;
#define FLOAT_FIELD(field_name) instance->field_name=0;
#define VECTOR_FIELD(field_name) vector_init(&instance->field_name, sizeof(Expression*), 8);
#define END \
        return instance; \
    }

AST_EXPRESSIONS

#undef EXPRESSION
#undef SPECIFIED_EXPRESSION_FIELD
#undef EXPRESSION_FIELD               
#undef BOOL_FIELD                   
#undef STRING_FIELD
#undef VECTOR_FIELD
#undef FLOAT_FIELD
#undef INT_FIELD
#undef FLOAT_FIELD
#undef INT_FIELD
#undef END

void allow_unused_variable(void* variable){}

char* stringify_expression(Expression* expression, int indentation){
    if(expression==NULL){
        return strdup("NULL");
    }

    stream s;
    stream_init(&s, STRINGIFY_BUFFER_SIZE);

    char* indentation_string=malloc(sizeof(char)*(indentation+1));
    CHECK_ALLOCATION(indentation_string);
    for(int i=0; i<indentation; i++){
        indentation_string[i]='\t';
    }
    indentation_string[indentation]='\0';

    #define WRITE(message) stream_push(&s, message, strlen(message));
    #define WRITE_CONST(message) stream_push(&s, message, sizeof(message));
    #define WRITE_FORMATTED(message, ...) stream_printf(&s, message, ##__VA_ARGS__);

    switch(expression->type){
        #define EXPRESSION(struct_name, type_tag) \
            case e_##type_tag: { \
                struct_name* casted=(struct_name*)expression; \
                allow_unused_variable((void*)casted); \
                WRITE_FORMATTED("\n%s"#struct_name": ", indentation_string)

        #define WRITE_FIELD(field_name, field_stringified) WRITE_FORMATTED("\n%s->" #field_name ": ", indentation_string) WRITE(field_stringified)
        #define WRITE_EXPRESSION_FIELD(field_name) \
            { char* str=stringify_expression((Expression*)casted->field_name, indentation+1); \
            WRITE_FIELD(field_name, str) \
            free(str); }
        
        #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) WRITE_EXPRESSION_FIELD(field_name)
        #define EXPRESSION_FIELD(field_name)                 WRITE_EXPRESSION_FIELD(field_name)
        #define BOOL_FIELD(field_name)                       WRITE_FIELD(field_name, casted->field_name ? "true" : "false")
        #define STRING_FIELD(field_name)                     if(casted->field_name!=NULL) { WRITE_FORMATTED("\"%s\"", casted->field_name) }
        #define FLOAT_FIELD(field_name)                      WRITE_FORMATTED("%f", casted->field_name)
        #define INT_FIELD(field_name)                        WRITE_FORMATTED("%i", casted->field_name)
        #define VECTOR_FIELD(field_name) \
            WRITE_FORMATTED("\n%s->" #field_name ": ", indentation_string) \
            for (int i = 0; i < vector_count(&casted->field_name); i++){ \
                WRITE(stringify_expression(pointers_vector_get(&casted->field_name, i), indentation+1)) \
            }
        #define END \
                break; \
            }
        AST_EXPRESSIONS
        default: INCORRECT_ENUM_VALUE(ExpressionType, expression, expression->type)

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
    WRITE_CONST("\0")

    #undef WRITE
    #undef WRITE_CONST
    #undef WRITE_FORMATTED
    #undef WRITE_FIELD
    #undef WRITE_EXPRESSION_FIELD

    free(indentation_string);
    stream_truncate(&s);
    return stream_get_data(&s); 
}

void delete_expression_keep_children(Expression* expression){
    switch(expression->type){
        #define EXPRESSION(struct_name, type_tag) \
            case e_##type_tag: {\
                struct_name* casted=(struct_name*)expression;
        #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name)
        #define EXPRESSION_FIELD(field_name)
        #define BOOL_FIELD(field_name)
        #define STRING_FIELD(field_name) free(casted->field_name);
        #define FLOAT_FIELD(field_name)
        #define INT_FIELD(field_name)
        #define VECTOR_FIELD(field_name) vector_deinit(&casted->field_name);
        #define END \
                free(casted); \
                break; \
            }
        default: INCORRECT_ENUM_VALUE(ExpressionType, expression, expression->type)
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

void delete_expression(Expression* expression){
    if(expression==NULL){
        return;
    }
    AST_DEALLOCATION
    switch(expression->type){
        #define EXPRESSION(struct_name, type_tag) \
            case e_##type_tag: {\
            struct_name* casted=(struct_name*)expression;
        #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) delete_expression((Expression*)casted->field_name);
        #define EXPRESSION_FIELD(field_name)                                  delete_expression(casted->field_name);
        #define BOOL_FIELD(field_name)
        #define STRING_FIELD(field_name)                     free(casted->field_name);
        #define FLOAT_FIELD(field_name)
        #define INT_FIELD(field_name)
        #define VECTOR_FIELD(field_name) \
            for (int i = 0; i < vector_count(&casted->field_name); i++){ \
                delete_expression(pointers_vector_get(&casted->field_name, i)); \
            } \
            vector_deinit(&casted->field_name);
        #define END \
                free(casted); \
                break; \
            }
        default: INCORRECT_ENUM_VALUE(ExpressionType, expression, expression->type)
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

Expression* copy_expression(Expression* expression){
    if(expression==NULL){
        return NULL;
    }
    switch(expression->type){
        #define EXPRESSION(struct_name, type_tag) \
            case e_##type_tag: { \
            struct_name* casted=(struct_name*)expression; \
            allow_unused_variable(casted); \
            struct_name* copy=new_##type_tag(); \
            copy->line_number=expression->line_number; \
            copy->column_number=expression->column_number;
        
        #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) \
                                                             copy->field_name=(struct_name*)copy_expression((Expression*)casted->field_name);
        #define EXPRESSION_FIELD(field_name)                 copy->field_name=copy_expression(casted->field_name);
        #define BOOL_FIELD(field_name)                       copy->field_name=casted->field_name;
        #define INT_FIELD(field_name)                        copy->field_name=casted->field_name;
        #define FLOAT_FIELD(field_name)                        copy->field_name=casted->field_name;
        #define STRING_FIELD(field_name)                     copy->field_name=strdup(casted->field_name);
        #define VECTOR_FIELD(field_name) \
            for (int i = 0; i < vector_count(&casted->field_name); i++){ \
                pointers_vector_push(&copy->field_name, copy_expression((Expression*)pointers_vector_get(&casted->field_name, i))); \
            }
        #define END \
                return (Expression*)copy; \
            }
        
        default: INCORRECT_ENUM_VALUE(ExpressionType, expression, expression->type)

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

bool compare_strings(char* a, char* b){
    if(a==NULL && b==NULL) {
        return true;
    } else if(a==NULL || b==NULL) {
        return false;
    } else {
        return strcmp(a, b)==0;
    }
}

bool expressions_equal(Expression* expression_a, Expression* expression_b){
    if(expression_a==NULL && expression_b==NULL){
        return true;
    } else if(expression_a==NULL || expression_b==NULL || expression_a->type!=expression_b->type){
        return false;
    }
    switch(expression_a->type){
        #define EXPRESSION(struct_name, type_tag) \
            case e_##type_tag: { \
            struct_name* a=(struct_name*)expression_a; \
            struct_name* b=(struct_name*)expression_b; \
            allow_unused_variable(a); \
            allow_unused_variable(b);

        #define COMPARISON(c) if(!(c)) { return false; }
        
        #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) \
                                                             COMPARISON(expressions_equal((Expression*)a->field_name, (Expression*)b->field_name))
        #define EXPRESSION_FIELD(field_name)                 COMPARISON(expressions_equal(a->field_name, b->field_name))
        #define BOOL_FIELD(field_name)                       COMPARISON(a->field_name==b->field_name)
        #define FLOAT_FIELD(field_name)                      COMPARISON(a->field_name==b->field_name)
        #define INT_FIELD(field_name)                        COMPARISON(a->field_name==b->field_name)
        #define STRING_FIELD(field_name)                     COMPARISON(compare_strings(a->field_name, b->field_name))
        #define VECTOR_FIELD(field_name) \
            COMPARISON(vector_count(&a->field_name)==vector_count(&b->field_name)) \
            for (int i = 0; i < vector_count(&a->field_name); i++){ \
                COMPARISON(expressions_equal(pointers_vector_get(&a->field_name, i), pointers_vector_get(&b->field_name, i)))  \
            }
        #define END \
                return true; \
            }
        
        default: INCORRECT_ENUM_VALUE(ExpressionType, expression_a, expression_a->type);

        AST_EXPRESSIONS

        #undef COMPARE
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