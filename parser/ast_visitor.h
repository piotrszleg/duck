#ifndef AST_VISITOR_H
#define AST_VISITOR_H

#include <stdbool.h>
#include "ast.h"

typedef enum MoveRequest MoveRequest;
enum MoveRequest{
    down,// default visitng order
    next,// don't visit the inner hierarchy
    up,// go up from nested expression
};

typedef struct ASTVisitorRequest ASTVisitorRequest;
struct ASTVisitorRequest{
    MoveRequest move;
    // if replacement is not null it will be visited and then appended
    // instead of the visited expression
    expression* replacement;
};

typedef ASTVisitorRequest (*visitor_function)(expression*, void*);

ASTVisitorRequest visit_ast(expression*, visitor_function f, void*);

#endif