#ifndef AST_H
#define AST_H

#include <string.h>
#include "../datatypes/vector.h"
#include "../datatypes/stream.h"
#include "../error/error.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef COUNT_AST_ALLOCATIONS
bool ast_allocations_zero();
#endif

#define AST_EXPRESSIONS \
    EXPRESSION(expression) \
    END \
    EXPRESSION(empty) \
    END \
    EXPRESSION(block) \
        VECTOR_FIELD(lines)\
    END \
    EXPRESSION(table_literal) \
        VECTOR_FIELD(lines)\
    END \
    EXPRESSION(path) \
        VECTOR_FIELD(lines)\
    END \
    EXPRESSION(name) \
        STRING_FIELD(value)\
    END \
    EXPRESSION(assignment) \
        SPECIFIED_EXPRESSION_FIELD(path, left)\
        EXPRESSION_FIELD(right) \
        BOOL_FIELD(used_in_closure) \
    END \
    EXPRESSION(function_call) \
        EXPRESSION_FIELD(called) \
        SPECIFIED_EXPRESSION_FIELD(table_literal, arguments) \
    END \
    EXPRESSION(binary) \
        EXPRESSION_FIELD(left)\
        STRING_FIELD(op) \
        EXPRESSION_FIELD(right) \
    END \
    EXPRESSION(prefix) \
        STRING_FIELD(op) \
        EXPRESSION_FIELD(right) \
    END \
    EXPRESSION(function_declaration) \
        VECTOR_FIELD(arguments) \
        BOOL_FIELD(variadic) \
        EXPRESSION_FIELD(body) \
    END \
    EXPRESSION(conditional) \
        EXPRESSION_FIELD(condition) \
        EXPRESSION_FIELD(ontrue) \
        EXPRESSION_FIELD(onfalse) \
    END \
    EXPRESSION(function_return) \
        EXPRESSION_FIELD(value) \
    END \
    EXPRESSION(parentheses) \
        EXPRESSION_FIELD(value) \
    END \
    EXPRESSION(message) \
        EXPRESSION_FIELD(messaged_object) \
        SPECIFIED_EXPRESSION_FIELD(name, message_name) \
        SPECIFIED_EXPRESSION_FIELD(table_literal, arguments) \
    END \
    EXPRESSION(macro) \
        SPECIFIED_EXPRESSION_FIELD(path, pth) \
    END \
    EXPRESSION(macro_declaration) \
        SPECIFIED_EXPRESSION_FIELD(macro, left) \
        EXPRESSION_FIELD(right) \
    END \
    EXPRESSION(literal) \
        LITERAL_UNION \
    END \

// generate enum of expression types, each value is prepended with "e_"
typedef enum expression_type expression_type;
enum expression_type{
    #define EXPRESSION(t) e_##t,
    #define SPECIFIED_EXPRESSION_FIELD(type, field_name)
    #define EXPRESSION_FIELD(field_name)
    #define BOOL_FIELD(field_name)    
    #define STRING_FIELD(field_name)
    #define VECTOR_FIELD(field_name)
    #define LITERAL_UNION
    #define END 

    AST_EXPRESSIONS

    #undef EXPRESSION
    #undef SPECIFIED_EXPRESSION_FIELD
    #undef EXPRESSION_FIELD               
    #undef BOOL_FIELD                   
    #undef STRING_FIELD
    #undef VECTOR_FIELD
    #undef LITERAL_UNION
    #undef END
};

#define EXPRESSION_TYPES_COUNT t_literal

typedef enum literal_type literal_type;
enum literal_type{ l_int, l_float, l_string };

#define EXPRESSION(t) \
    typedef struct t t; \
    t* new_ ## t(); \
    struct t { \
        expression_type type; \
        int line_number; \
        int column_number; \

#define SPECIFIED_EXPRESSION_FIELD(type, field_name) type* field_name;
#define EXPRESSION_FIELD(field_name)                 expression* field_name;
#define BOOL_FIELD(field_name)                       bool field_name;
#define STRING_FIELD(field_name)                     char* field_name;
#define VECTOR_FIELD(field_name)                     vector field_name;
#define LITERAL_UNION \
        literal_type ltype; \
        union { \
            int ival; \
            float fval; \
            char* sval; \
        };
#define END \
    };

AST_EXPRESSIONS

#undef EXPRESSION
#undef SPECIFIED_EXPRESSION_FIELD
#undef EXPRESSION_FIELD               
#undef BOOL_FIELD                   
#undef STRING_FIELD
#undef VECTOR_FIELD
#undef LITERAL_UNION
#undef END

bool check_expression(expression* e);
char* stringify_expression(expression*, int);
void delete_expression(expression*);
expression* copy_expression(expression*);
char* table_literal_extract_key(assignment* a);
bool expressions_equal(expression* expression_a, expression* expression_b);

#endif