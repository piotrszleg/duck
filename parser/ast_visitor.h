#ifndef AST_VISITOR_H
#define AST_VISITOR_H

#include <stdbool.h>
#include "ast.h"

typedef enum move_request move_request;
enum move_request{
    down,// default visitng order
    next,// don't visit the inner hierarchy
    up,// go up from nested expression
};

typedef struct ast_visitor_request ast_visitor_request;
struct ast_visitor_request{
    move_request move;
    // if replacement is not null it will be visited and then appended
    // instead of the visited expression
    expression* replacement;
};

typedef ast_visitor_request (*visitor_function)(expression*, void*);

ast_visitor_request visit_ast(expression*, visitor_function f, void*);

#endif