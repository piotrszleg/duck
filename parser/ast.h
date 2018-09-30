#ifndef AST_H
#define AST_H

#include <string.h>
#include "vector.h"
#include <stdlib.h>
#include <stdio.h>

typedef enum expression_type expression_type;
enum expression_type{
    _empty,
    _block,
    _literal,
    _name,
    _assignment,
    _function_call,
    _unary,
    _function_declaration,
    _conditional
};

typedef struct expression expression;
struct expression {
    expression_type type;
};

#define AST_OBJECT(t, body) \
    typedef struct t t; \
    struct t { \
        expression_type type; \
        body \
    }; \
    t* new_ ## t(); \

AST_OBJECT(empty,)

AST_OBJECT(block, 
    vector lines;
)

AST_OBJECT(name, 
    char* value;
)


AST_OBJECT(assignment,
    name* left;
    expression* right;
)

AST_OBJECT(function_call,
    name* function_name;
    block* arguments;
)

AST_OBJECT(unary,
    expression* left;
    char op;
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