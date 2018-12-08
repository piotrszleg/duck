#include "ast_optimisations.h"

move_request constants_folding (expression* exp, void* data){
    switch(exp->type){
        case _unary:
        {
            unary* u=(unary*)exp;

            u->right;// if it is constant evaluate it and push the result instead of the unary
            u->left;
            break;
        }
        case _prefix:
        {
            prefix* p=(prefix*)exp;
            p->right;
            break;
        }
    }
}