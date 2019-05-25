#include "ast.h"

#define STRINGIFY_BUFFER_SIZE 1024

#define X(t) \
    t* new_ ## t(){  \
        t* instance=malloc(sizeof(t)); \
        CHECK_ALLOCATION(instance); \
		instance->type=e_ ## t; \
        return instance; \
    }
    EXPRESSION_TYPES
#undef X

bool check_expression(expression* e){
	return e!=NULL && e->type>=e_empty && e->type<=e_function_return;
}

void string_replace(char *s, char from, char to) {
    while (*s == from)
    *s++ = to;
}

char* stringify_expression(expression* exp, int indentation){

    if(exp==NULL){
        return strdup("NULL");
    }

    char* result=malloc(sizeof(char)*(STRINGIFY_BUFFER_SIZE+1));
    CHECK_ALLOCATION(result);
    result[0]='\0';

    char* indentation_string=malloc(sizeof(char)*(indentation+1));
    CHECK_ALLOCATION(indentation_string);
    if(indentation>0){
        sprintf(indentation_string, "%0*d", indentation, 0);
        string_replace(indentation_string, '0', '\t');
    } else {
        strcpy(indentation_string, "\0");
    }

    switch(exp->type){
        case e_empty:
            strcat(result, "EMPTY\0");// "EMPTY" will always smaller than STRINGIFY_BUFFER_SIZE
            break;
        case e_name:
            snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sNAME: %s", indentation_string, ((name*)exp)->value);
            break;
        case e_literal:
        {
            literal* l=(literal*)exp;
            switch(l->ltype){
                case l_int:
                    snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sLITERAL: %i", indentation_string, l->ival);
                    break;
                case l_float:
                    snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sLITERAL: %f", indentation_string, l->fval);
                    break;
                case l_string:
                    snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sLITERAL: %s", indentation_string, l->sval);
                    break;
            }
            break;
        }
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            char* used_in_closure= a->used_in_closure ? "(used_in_closure)" : "";
            snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sASSIGNMENT%s: \n%s-> left: %s \n%s-> sign: '=' \n%s-> right: %s", 
                                        indentation_string,
                                        used_in_closure,
                                        indentation_string, stringify_expression((expression*)a->left, indentation+1), 
                                        indentation_string, 
                                        indentation_string, stringify_expression((expression*)a->right, indentation+1));
            break;
        }
        case e_unary:
        {
            unary* u=(unary*)exp;
            snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sUNARY: \n%s-> left: %s \n%s-> sign: '%s' \n%s-> right: %s", 
                                        indentation_string,
                                        indentation_string, stringify_expression((expression*)u->left, indentation+1), 
                                        indentation_string, u->op,
                                        indentation_string, stringify_expression((expression*)u->right, indentation+1));
            break;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sPREFIX: \n%s-> sign: '%s' \n%s-> right: %s", 
                                        indentation_string,
                                        indentation_string, p->op,
                                        indentation_string, stringify_expression((expression*)p->right, indentation+1));
            break;
        }
        case e_function_call:
        {
            function_call* c=(function_call*)exp;
            snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sCALL: \n%s-> name: %s \n%s-> arguments: %s", 
                                        indentation_string,
                                        indentation_string, stringify_expression((expression*)c->called, indentation+1), 
                                        indentation_string, stringify_expression((expression*)c->arguments, indentation+1));
            break;
        }
        case e_message:
        {
            message* m=(message*)exp;
            snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sMESSSAGE: \n%s-> object: %s \n%s-> name: %s \n%s-> arguments: %s", 
                                        indentation_string,
                                        indentation_string, stringify_expression((expression*)m->messaged_object, indentation+1), 
                                        indentation_string, stringify_expression((expression*)m->message_name, indentation+1), 
                                        indentation_string, stringify_expression((expression*)m->arguments, indentation+1));
            break;
        }
        #define STRINGIFY_LINES \
            block* b=(block*)exp; \
            for (int i = 0; i < vector_total(&b->lines); i++){ \
                strcat(result, stringify_expression(vector_get(&b->lines, i), indentation+1)); \
            }
        case e_block:
        {
            snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sBLOCK: ", indentation_string);
            STRINGIFY_LINES
            break;
        }
        case e_table_literal:
        {
            snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sTABLE_LITERAL: ", indentation_string);
            STRINGIFY_LINES
            break;
        }
        case e_path:
        {
            if(exp->type==e_path)snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sPATH: ", indentation_string);
            STRINGIFY_LINES
            break;
        }
        #undef STRINGIFY_LINES
        case e_function_declaration:
        {
            function_declaration* f=(function_declaration*)exp;
            char* stringified_arguments=calloc(STRINGIFY_BUFFER_SIZE+1, sizeof(char));
            for (int i = 0; i < vector_total(f->arguments); i++){
                strcat(stringified_arguments, stringify_expression(vector_get(f->arguments, i), indentation+1));
            }
            snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sFUNCTION: \n%s-> arguments: %s \n%s-> body: %s", 
                                        indentation_string,
                                        indentation_string, stringified_arguments,
                                        indentation_string, stringify_expression((expression*)f->body, indentation+1)
                                        );
            free(stringified_arguments);
            break;
        }
        case e_conditional:
        {
            conditional* c=(conditional*)exp;
            snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sCONDITIONAL: \n%s-> condition: %s \n%s-> ontrue: %s \n%s-> onfalse: %s", 
                                        indentation_string,
                                        indentation_string, stringify_expression((expression*)c->condition, indentation+1),
                                        indentation_string, stringify_expression((expression*)c->ontrue, indentation+1),
                                        indentation_string, stringify_expression((expression*)c->onfalse, indentation+1)
                                        );
            break;
        }
        case e_function_return:
        {
            function_return* r=(function_return*)exp;
            snprintf(result, STRINGIFY_BUFFER_SIZE, "\n%sFUNCTION_RETURN: \n%s-> value: %s", 
                                        indentation_string,
                                        indentation_string, stringify_expression((expression*)r->value, indentation+1)
                                        );
            break;
        }
        default:
            strcat(result, "Undefined stringification");
    }
    free(indentation_string);
    char* result_truncated=strdup(result);
    free(result);
    return result_truncated; 
}

void delete_expression(expression* exp){

    if(exp==NULL){
        printf("delete_expression argument is null.");
    }

    switch(exp->type){
        case e_empty:
            free((empty*)exp);
            break;
        case e_name:
            free(((name*)exp)->value);
            free(((name*)exp));
            break;
        case e_literal:
        {
            literal* l=(literal*)exp;
            if(l->ltype==l_string){
                free(l->sval);// only char array pointer needs freeing
            }
            free(l);
            break;
        }
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            delete_expression((expression*)a->left);
            delete_expression((expression*)a->right);
            free(a);
            break;
        }
        case e_unary:
        {
            unary* u=(unary*)exp;
            delete_expression((expression*)u->left);
            delete_expression((expression*)u->right);
            free(u->op);
            free(u);
            break;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            delete_expression((expression*)p->right);
            free(p->op);
            free(p);
            break;
        }
        case e_function_call:
        {
            function_call* c=(function_call*)exp;
            delete_expression((expression*)c->called);
            if(c->arguments!=NULL){
                delete_expression((expression*)c->arguments);
            }
            free(c);
            break;
        }
        case e_message:
        {
            message* m=(message*)exp;
            delete_expression((expression*)m->messaged_object);
            delete_expression((expression*)m->message_name);
            if(m->arguments!=NULL){
                delete_expression((expression*)m->arguments);
            }
            free(m);
            break;
        }
        case e_block:
        case e_table_literal:
        case e_path:
        {
            block* b=(block*)exp;// block, table_literal and path have the same memory layout, the only difference is type tag
            for (int i = 0; i < vector_total(&b->lines); i++){
                delete_expression(vector_get(&b->lines, i));
            }
            vector_free(&b->lines);
            free(b);
            break;
        }
        case e_function_declaration:
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
        case e_conditional:
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
        case e_function_return:
        {
            function_return* r=(function_return*)exp;
            delete_expression((expression*)r->value);
            free(r);
            break;
        }
        default:
            printf("Deletion of this type (%i) was not implemented", exp->type);
            break;
    }
}

expression* copy_expression(expression* exp){
    if(exp==NULL){
        printf("copy_expression argument is null.");
    }

    #define COPY_LOCATION copy->line_number=exp->line_number; copy->column_number=exp->column_number;

    switch(exp->type){
        case e_empty:
            return (expression*)new_empty();
        case e_name:
        {
            name* copy=new_name();
            COPY_LOCATION
            copy->line_number=exp->line_number;
            ((name*)copy)->value=strdup(((name*)exp)->value);
            return (expression*)copy;
        }
        case e_literal:
        {
            literal* l=(literal*)exp;
            literal* copy=new_literal();
            COPY_LOCATION
            copy->ltype=l->ltype;
            switch(l->ltype){
                case l_string:
                    copy->sval=strdup(l->sval);// only char array pointer needs freeing
                    break;
                case l_float:
                    copy->fval=l->fval;
                    break;
                case l_int:
                    copy->ival=l->ival;
                    break;
            }
            return (expression*)copy;
        }
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            assignment* copy=new_assignment();
            COPY_LOCATION
            copy->left=(path*)copy_expression((expression*)a->left);
            copy->right=copy_expression(a->right);
            return (expression*)copy;
        }
        case e_unary:
        {
            unary* u=(unary*)exp;
            unary* copy=new_unary();
            COPY_LOCATION

            copy->left=copy_expression(u->left);
            copy->right=copy_expression(u->right);
            copy->op=strdup(u->op);
            return (expression*)copy;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            prefix* copy=new_prefix();
            COPY_LOCATION

            copy->right=copy_expression(p->right);
            copy->op=strdup(p->op);
            return (expression*)copy;
        }
        case e_function_call:
        {
            function_call* c=(function_call*)exp;
            function_call* copy=new_function_call();
            COPY_LOCATION

            copy->called=copy_expression((expression*)c->called);
            if(c->arguments!=NULL){
                copy->arguments=(table_literal*)copy_expression((expression*)c->arguments);
            }
            return (expression*)copy;
        }
        case e_block:
        case e_table_literal:
        case e_path:
        {
            block* b=(block*)exp;// block, table_literal and path have the same memory layout, the only difference is type tag
            block* copy=new_block();
            COPY_LOCATION
            vector_init(&copy->lines);
            for (int i = 0; i < vector_total(&b->lines); i++){
                vector_add(&copy->lines, copy_expression(vector_get(&b->lines, i)));
            }
            copy->type=exp->type;
            return (expression*)copy;
        }
        case e_function_declaration:
        {
            function_declaration* f=(function_declaration*)exp;
            function_declaration* copy=new_function_declaration();
            copy->variadic=f->variadic;
            COPY_LOCATION
            copy->arguments=malloc(sizeof(vector));
            vector_init(copy->arguments);
            for (int i = 0; i < vector_total(f->arguments); i++){
                vector_add(copy->arguments, copy_expression(vector_get(f->arguments, i)));
            }
            copy->body=copy_expression(f->body);
            return (expression*)copy;
        }
        case e_conditional:
        {
            conditional* c=(conditional*)exp;
            conditional* copy=new_conditional();
            COPY_LOCATION

            copy->condition=copy_expression(c->condition);
            copy->ontrue=copy_expression(c->ontrue);
            
            if(c->onfalse!=NULL){
                copy->onfalse=copy_expression(c->onfalse);
            }
            return (expression*)copy;
        }
        case e_function_return:
        {
            function_return* r=(function_return*)exp;
            function_return* copy=new_function_return();
            COPY_LOCATION
            copy->value=copy_expression(r->value);
            return (expression*)copy;
        }
        default:
            printf("Copying of this type (%i) was not implemented", exp->type);
            return NULL;
    }
}