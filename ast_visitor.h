#ifndef AST_VISITOR_H
#define AST_VISITOR_H

#include <ast.h>

typedef enum move_request move_request;
enum move_request{
    down,
    next,
    up,
};

typedef move_request (*visitor_function)(expression*, void*);

move_request visit_ast(expression*, visitor_function f, void*);

#endif