#include "ast.h"

#define STRINGIFY_BUFFER_SIZE 64

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

#define EXPRESSION(t) \
t* new_ ## t(){  \
    t* instance=malloc(sizeof(t)); \
    CHECK_ALLOCATION(instance); \
    AST_ALLOCATION \
	instance->type=e_ ## t; \

#define SPECIFIED_EXPRESSION_FIELD(type, field_name) instance->field_name=(type*)NULL;
#define EXPRESSION_FIELD(field_name) instance->field_name=NULL;
#define BOOL_FIELD(field_name) instance->field_name=false;
#define STRING_FIELD(field_name) instance->field_name=NULL;
#define INT_FIELD(field_name) instance->field_name=0;
#define FLOAT_FIELD(field_name) instance->field_name=0;
#define VECTOR_FIELD(field_name) vector_init(&instance->field_name);
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

char* stringify_expression(expression* exp, int indentation){
    if(exp==NULL){
        return strdup("NULL");
    }

    stream s;
    init_stream(&s, STRINGIFY_BUFFER_SIZE);

    char* indentation_string=malloc(sizeof(char)*(indentation+1));
    CHECK_ALLOCATION(indentation_string);
    for(int i=0; i<indentation; i++){
        indentation_string[i]='\t';
    }
    indentation_string[indentation]='\0';

    char buffer[STRINGIFY_BUFFER_SIZE];

    #define WRITE(message) stream_push(&s, message, strlen(message));
    #define WRITE_CONST(message) stream_push(&s, message, sizeof(message));
    #define WRITE_FORMATTED(message, ...) snprintf(buffer, STRINGIFY_BUFFER_SIZE, message, ##__VA_ARGS__); WRITE(buffer)// TODO: upscale the buffer

    switch(exp->type){
        #define EXPRESSION(type) \
            case e_##type: { \
                type* casted=(type*)exp; \
                allow_unused_variable((void*)casted); \
                WRITE_FORMATTED("\n%s"#type": ", indentation_string)

        #define WRITE_FIELD(field_name, field_stringified) WRITE_FORMATTED("\n%s->" #field_name ": ", indentation_string) WRITE(field_stringified)
        #define WRITE_EXPRESSION_FIELD(field_name) \
            { char* str=stringify_expression((expression*)casted->field_name, indentation+1); \
            WRITE_FIELD(field_name, str) \
            free(str); }
        
        #define SPECIFIED_EXPRESSION_FIELD(type, field_name) WRITE_EXPRESSION_FIELD(field_name)
        #define EXPRESSION_FIELD(field_name)                 WRITE_EXPRESSION_FIELD(field_name)
        #define BOOL_FIELD(field_name)                       WRITE_FIELD(field_name, casted->field_name ? "true" : "false")
        #define STRING_FIELD(field_name)                     if(casted->field_name!=NULL) { WRITE_FIELD(field_name, casted->field_name) }
        #define FLOAT_FIELD(field_name)                      WRITE_FORMATTED("%f", casted->field_name)
        #define INT_FIELD(field_name)                        WRITE_FORMATTED("%i", casted->field_name)
        #define VECTOR_FIELD(field_name) \
            WRITE_FORMATTED("\n%s->" #field_name ": ", indentation_string) \
            for (int i = 0; i < vector_total(&casted->field_name); i++){ \
                WRITE(stringify_expression(vector_get(&casted->field_name, i), indentation+1)) \
            }
        #define END \
                break; \
            }
        AST_EXPRESSIONS
        default: THROW_ERROR(AST_ERROR, "Can't stringify expression of type %i", exp->type);

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

void delete_expression_keep_children(expression* exp){
    switch(exp->type){
        #define EXPRESSION(type) \
            case e_##type: {\
                type* casted=(type*)exp;
        #define SPECIFIED_EXPRESSION_FIELD(type, field_name)
        #define EXPRESSION_FIELD(field_name)
        #define BOOL_FIELD(field_name)
        #define STRING_FIELD(field_name)                     free(casted->field_name);
        #define FLOAT_FIELD(field_name)
        #define INT_FIELD(field_name)
        #define VECTOR_FIELD(field_name)                     vector_free(&casted->field_name);
        #define END \
                free(casted); \
                break; \
            }
        default: THROW_ERROR(AST_ERROR, "Can't delete expression of type %i", exp->type);

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

void delete_expression(expression* exp){
    if(exp==NULL){
        return;
    }
    AST_DEALLOCATION
    switch(exp->type){
        #define EXPRESSION(type) \
            case e_##type: {\
            type* casted=(type*)exp;
        #define SPECIFIED_EXPRESSION_FIELD(type, field_name) delete_expression((expression*)casted->field_name);
        #define EXPRESSION_FIELD(field_name)                 delete_expression(casted->field_name);
        #define BOOL_FIELD(field_name)
        #define STRING_FIELD(field_name)                     free(casted->field_name);
        #define FLOAT_FIELD(field_name)
        #define INT_FIELD(field_name)
        #define VECTOR_FIELD(field_name) \
            for (int i = 0; i < vector_total(&casted->field_name); i++){ \
                delete_expression(vector_get(&casted->field_name, i)); \
            } \
            vector_free(&casted->field_name);
        #define END \
                free(casted); \
                break; \
            }
        default: THROW_ERROR(AST_ERROR, "Can't delete expression of type %i", exp->type);

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

expression* copy_expression(expression* exp){
    if(exp==NULL){
        return NULL;
    }
    switch(exp->type){
        #define EXPRESSION(type) \
            case e_##type: { \
            type* casted=(type*)exp; \
            allow_unused_variable(casted); \
            type* copy=new_##type(); \
            copy->line_number=exp->line_number; \
            copy->column_number=exp->column_number;
        
        #define SPECIFIED_EXPRESSION_FIELD(type, field_name) copy->field_name=(type*)copy_expression((expression*)casted->field_name);
        #define EXPRESSION_FIELD(field_name)                 copy->field_name=copy_expression(casted->field_name);
        #define BOOL_FIELD(field_name)                       copy->field_name=casted->field_name;
        #define INT_FIELD(field_name)                        copy->field_name=casted->field_name;
        #define FLOAT_FIELD(field_name)                        copy->field_name=casted->field_name;
        #define STRING_FIELD(field_name)                     copy->field_name=strdup(casted->field_name);
        #define VECTOR_FIELD(field_name) \
            for (int i = 0; i < vector_total(&casted->field_name); i++){ \
                vector_add(&copy->field_name, copy_expression((expression*)vector_get(&casted->field_name, i))); \
            }
        #define END \
                return (expression*)copy; \
            }
        
        default: THROW_ERROR(AST_ERROR, "Can't copy expression of type %i", exp->type);

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

bool expressions_equal(expression* expression_a, expression* expression_b){
    if(expression_a==NULL && expression_b==NULL){
        return true;
    } else if(expression_a==NULL || expression_b==NULL || expression_a->type!=expression_b->type){
        return false;
    }
    switch(expression_a->type){
        #define EXPRESSION(type) \
            case e_##type: { \
            type* a=(type*)expression_a; \
            type* b=(type*)expression_b; \
            allow_unused_variable(a); \
            allow_unused_variable(b);

        #define COMPARISON(c) if(!(c)) { return false; }
        
        #define SPECIFIED_EXPRESSION_FIELD(type, field_name) COMPARISON(expressions_equal((expression*)a->field_name, (expression*)b->field_name))
        #define EXPRESSION_FIELD(field_name)                 COMPARISON(expressions_equal(a->field_name, b->field_name))
        #define BOOL_FIELD(field_name)                       COMPARISON(a->field_name==b->field_name)
        #define FLOAT_FIELD(field_name)                      COMPARISON(a->field_name==b->field_name)
        #define INT_FIELD(field_name)                        COMPARISON(a->field_name==b->field_name)
        #define STRING_FIELD(field_name)                     COMPARISON(compare_strings(a->field_name, b->field_name))
        #define VECTOR_FIELD(field_name) \
            COMPARISON(vector_total(&a->field_name)==vector_total(&b->field_name)) \
            for (int i = 0; i < vector_total(&a->field_name); i++){ \
                COMPARISON(expressions_equal(vector_get(&a->field_name, i), vector_get(&b->field_name, i)))  \
            }
        #define END \
                return true; \
            }
        
        default: THROW_ERROR(AST_ERROR, "Can't compare expressions of type %i", expression_a->type);

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

char* table_literal_extract_key(assignment* a){
    if(vector_total(&a->left->lines)!=1) {
        THROW_ERROR(BYTECODE_ERROR, "Table literal key should have only one name in path.");
        return NULL;
    }
    expression* e=vector_get(&a->left->lines, 0);
    if(e->type!=e_name) {
        THROW_ERROR(BYTECODE_ERROR, "Table literal key should be of type name.");
        return NULL;
    }
    return ((name*)e)->value;
}