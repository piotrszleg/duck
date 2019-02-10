#ifndef AST_VISITOR_H
#define AST_VISITOR_H

#include <stdbool.h>
#include "../parser/ast.h"

typedef enum move_request move_request;
enum move_request{
    down,
    next,
    up,
};

typedef struct ast_visitor_request ast_visitor_request;
struct ast_visitor_request{
    move_request move;
    expression* replacement;
};

typedef ast_visitor_request (*visitor_function)(expression*, void*);

ast_visitor_request visit_ast(expression*, visitor_function f, void*);

#endif