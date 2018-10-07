#include "ast.h"

#define AST_OBJECT_NEW(t) \
    t* new_ ## t(){  \
        t* instance=malloc(sizeof(t)); \
		instance->type=_ ## t; \
        return instance; \
    }

AST_OBJECT_NEW(empty)
AST_OBJECT_NEW(block)
AST_OBJECT_NEW(literal)
AST_OBJECT_NEW(name)
AST_OBJECT_NEW(assignment)
AST_OBJECT_NEW(function_call)
AST_OBJECT_NEW(unary)
AST_OBJECT_NEW(prefix)
AST_OBJECT_NEW(function_declaration)
AST_OBJECT_NEW(conditional)

void string_replace(char *s, char from, char to) {
    while (*s == from)
    *s++ = to;
}

char* stringify_expression(expression* exp, int indentation){

    if(exp==NULL){
        return "NULL";
    }

    int result_size=1024;
    char* result=calloc(result_size+1, sizeof(char));

    char* indentation_string=malloc(sizeof(char)*(indentation+1));
    if(indentation>0){
        sprintf(indentation_string, "%0*d", indentation, 0);
        string_replace(indentation_string, '0', '\t');
    } else {
        strcpy(indentation_string, "\0");
    }

    switch(exp->type){
        case _empty:
            strcat(result, "EMPTY\0");// "EMPTY" will always smaller than result_size
            break;
        case _name:
            snprintf(result, result_size, "\n%sNAME: %s", indentation_string, ((name*)exp)->value);
            break;
        case _literal:
        {
            literal* l=(literal*)exp;
            switch(l->ltype){
                case _int:
                    snprintf(result, result_size, "\n%sLITERAL: %i", indentation_string, l->ival);
                    break;
                case _float:
                    snprintf(result, result_size, "\n%sLITERAL: %f", indentation_string, l->fval);
                    break;
                case _string:
                    snprintf(result, result_size, "\n%sLITERAL: %s", indentation_string, l->sval);
                    break;
            }
            break;
        }
        case _assignment:
        {
            assignment* a=(assignment*)exp;
            snprintf(result, result_size, "\n%sASSIGNMENT: \n%s-> left: %s \n%s-> sign: '=' \n%s-> right: %s", 
                                        indentation_string,
                                        indentation_string, stringify_expression((expression*)a->left, indentation+1), 
                                        indentation_string, 
                                        indentation_string, stringify_expression((expression*)a->right, indentation+1));
            break;
        }
        case _unary:
        {
            unary* u=(unary*)exp;
            snprintf(result, result_size, "\n%sUNARY: \n%s-> left: %s \n%s-> sign: '%s' \n%s-> right: %s", 
                                        indentation_string,
                                        indentation_string, stringify_expression((expression*)u->left, indentation+1), 
                                        indentation_string, u->op,
                                        indentation_string, stringify_expression((expression*)u->right, indentation+1));
            break;
        }
        case _prefix:
        {
            prefix* p=(prefix*)exp;
            snprintf(result, result_size, "\n%sUNARY: \n%s-> sign: '%s' \n%s-> right: %s", 
                                        indentation_string,
                                        indentation_string, p->op,
                                        indentation_string, stringify_expression((expression*)p->right, indentation+1));
            break;
        }
        case _function_call:
        {
            function_call* c=(function_call*)exp;
            snprintf(result, result_size, "\n%sCALL: \n%s-> name: %s \n%s-> arguments: %s", 
                                        indentation_string,
                                        indentation_string, stringify_expression((expression*)c->function_name, indentation+1), 
                                        indentation_string, stringify_expression((expression*)c->arguments, indentation+1));
            break;
        }
        case _block:
        {
            block* b=(block*)exp;
            snprintf(result, result_size, b->is_table ? "\n%sBLOCK: " : "\n%sTABLE: ", indentation_string);
            for (int i = 0; i < vector_total(&b->lines); i++){
                strcat(result, stringify_expression(vector_get(&b->lines, i), indentation+1));
            }
            break;
        }
        case _function_declaration:
        {
            function_declaration* f=(function_declaration*)exp;
            char* stringified_arguments=calloc(result_size+1, sizeof(char));
            for (int i = 0; i < vector_total(f->arguments); i++){
                strcat(stringified_arguments, stringify_expression(vector_get(f->arguments, i), indentation+1));
            }
            snprintf(result, result_size, "\n%sFUNCTION: \n%s-> arguments: %s \n%s-> body: %s", 
                                        indentation_string,
                                        indentation_string, stringified_arguments,
                                        indentation_string, stringify_expression((expression*)f->body, indentation+1)
                                        );
            break;
        }
        case _conditional:
        {
            conditional* c=(conditional*)exp;
            snprintf(result, result_size, "\n%sCONDITIONAL: \n%s-> condition: %s \n%s-> ontrue: %s \n%s-> onfalse: %s", 
                                        indentation_string,
                                        indentation_string, stringify_expression((expression*)c->condition, indentation+1),
                                        indentation_string, stringify_expression((expression*)c->ontrue, indentation+1),
                                        indentation_string, stringify_expression((expression*)c->onfalse, indentation+1)
                                        );
            break;
        }
        default:
            strcat(result, "Undefined stringification");
    }
    free(indentation_string);
    return result;    
}

void delete_expression(expression* exp){

    if(exp==NULL){
        printf("delete_expression argument is null.");
    }

    switch(exp->type){
        case _empty:
            free((empty*)exp);
            break;
        case _name:
            free(((name*)exp)->value);
            free(((name*)exp));
            break;
        case _literal:
        {
            literal* l=(literal*)exp;
            if(l->ltype==_string){
                free(l->sval);// only char array pointer needs freeing
            }
            free(l);
            break;
        }
        case _assignment:
        {
            assignment* a=(assignment*)exp;
            delete_expression((expression*)a->left);
            delete_expression((expression*)a->right);
            free(a);
            break;
        }
        case _unary:
        {
            unary* u=(unary*)exp;
            delete_expression((expression*)u->left);
            delete_expression((expression*)u->right);
            free(u->op);
            free(u);
            break;
        }
        case _prefix:
        {
            prefix* p=(prefix*)exp;
            delete_expression((expression*)p->right);
            free(p->op);
            free(p);
            break;
        }
        case _function_call:
        {
            function_call* c=(function_call*)exp;
            delete_expression((expression*)c->function_name);
            if(c->arguments!=NULL){
                delete_expression((expression*)c->arguments);
            }
            free(c);
            break;
        }
        case _block:
        {
            block* b=(block*)exp;
            for (int i = 0; i < vector_total(&b->lines); i++){
                delete_expression(vector_get(&b->lines, i));
            }
            vector_free(&b->lines);
            free(b);
            break;
        }
        case _function_declaration:
        {
            function_declaration* f=(function_declaration*)exp;
            for (int i = 0; i < vector_total(f->arguments); i++){
                delete_expression(vector_get(f->arguments, i));
            }
            free(f->arguments);
            delete_expression((expression*)f->body);
            free(f);
            break;
        }
        case _conditional:
        {
            conditional* c=(conditional*)exp;
            delete_expression((expression*)c->condition);
            delete_expression((expression*)c->ontrue);
            if(c->onfalse!=NULL){
                delete_expression((expression*)c->onfalse);
            }
            free(c);
            break;
        }
        default:
            printf("Deletion of this type (%i) was not implemented", exp->type);
            break;
    }
}