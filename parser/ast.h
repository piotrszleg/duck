#ifndef AST_H
#define AST_H

#include <string.h>
#include "../datatypes/vector.h"
#include "../error/error.h"
#include <stdlib.h>
#include <stdio.h>

typedef enum expression_type expression_type;
enum expression_type{
    e_empty,
    e_block,
    e_table_literal,
    e_path,
    e_literal,
    e_name,
    e_assignment,
    e_function_call,
    e_unary,
    e_prefix,
    e_function_declaration,
    e_conditional,
    e_function_return
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
    block* body;
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
enum literal_type{ _int, _float, _string };

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

#endif