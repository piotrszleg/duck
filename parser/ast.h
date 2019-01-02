#ifndef AST_H
#define AST_H

#include <string.h>
#include "../datatypes/vector.h"
#include "../error/error.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define EXPRESSION_TYPES_COUNT 13
#define EXPRESSION_TYPES \
    X(empty) \
    X(block) \
    X(table_literal) \
    X(path) \
    X(literal) \
    X(name) \
    X(assignment) \
    X(function_call) \
    X(unary) \
    X(prefix) \
    X(function_declaration) \
    X(conditional) \
    X(function_return) \

// generate enum of expression types, each value is prepended with "e_"
typedef enum expression_type expression_type;
enum expression_type{
    #define X(t) e_##t,
    EXPRESSION_TYPES
    #undef X
};

#define AST_OBJECT(t, body) \
    typedef struct t t; \
    struct t { \
        expression_type type; \
        int line_number; \
        int column_number; \
        body \
    }; \
    t* new_ ## t(); \

AST_OBJECT(expression,)

AST_OBJECT(empty,)

AST_OBJECT(block, 
    vector lines;
)

AST_OBJECT(table_literal, 
    vector lines;
)

AST_OBJECT(path,
    vector lines;
)

AST_OBJECT(name, 
    char* value;
)

AST_OBJECT(assignment,
    path* left;
    expression* right;
)

AST_OBJECT(function_call,
    path* function_path;
    table_literal* arguments;
)

AST_OBJECT(unary,
    expression* left;
    char* op;
    expression* right;
)

AST_OBJECT(prefix,
    char* op;
    expression* right;
)

AST_OBJECT(function_declaration,
    vector* arguments;
    bool variadic;
    expression* body;
)

AST_OBJECT(conditional,
    expression* condition;
    expression* ontrue;
    expression* onfalse;
)

AST_OBJECT(function_return,
    expression* value;
)

typedef enum literal_type literal_type;
enum literal_type{ l_int, l_float, l_string };

AST_OBJECT(literal,
    literal_type ltype;
    union {
        int ival;
        float fval;
        char* sval;
    };
)

char* stringify_expression(expression*, int);
void delete_expression(expression*);
expression* copy_expression(expression*);

#endif