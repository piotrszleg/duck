#include "ast_to_source.h"

void ast_to_source_recursive(stream* s, Expression* expression);

char* ast_to_source(Expression* expression){
    stream s;
    stream_init(&s, 16);
    ast_to_source_recursive(&s, expression);
    stream_push_const_string(&s, "\0");
    return stream_get_data(&s);
}

void arguments_to_source(stream* s, vector* arguments){
    stream_push_const_string(s, "(");
    for(int i=0; i<vector_count(arguments); i++){
        if(i!=0){
            stream_push_const_string(s, ", ");
        }
        ast_to_source_recursive(s, pointers_vector_get(arguments, i));
    }
    stream_push_const_string(s, ")");
}

void ast_to_source_recursive(stream* s, Expression* expression){
    switch(expression->type){
        case e_null_literal:
        case e_empty:          stream_push_const_string(s, "null");                             break;
        case e_int_literal:    stream_printf(s, "%i", ((IntLiteral*)expression)->value);        break;
        case e_float_literal:  stream_printf(s, "%f", ((FloatLiteral*)expression)->value);      break;
        case e_string_literal: stream_printf(s, "\"%s\"", ((StringLiteral*)expression)->value); break;
        case e_name:
            stream_push_string(s, ((Name*)expression)->value);
            break;
        case e_function_return:
             ast_to_source_recursive(s, ((FunctionReturn*)expression)->value);
             break;
        case e_parentheses:
            stream_push_const_string(s, "(");
            ast_to_source_recursive(s, ((Parentheses*)expression)->value);
            stream_push_const_string(s, ")");
            break;
        case e_unpack:
            stream_push_const_string(s, "...");
            ast_to_source_recursive(s, ((Unpack*)expression)->value);
            break;
        case e_argument:
            stream_push_string(s, ((Argument*)expression)->name);
            break;
        case e_variadic_argument:
            stream_push_string(s, ((Argument*)expression)->name);
            stream_push_const_string(s, "...");
            break;
        case e_optional_argument: {
            OptionalArgument* argument=(OptionalArgument*)expression;
            stream_push_string(s, argument->name);
            stream_push_const_string(s, "=");
            ast_to_source_recursive(s, argument->value);
            break;
        }
        case e_assignment: {
            Assignment* assignment=(Assignment*)expression;
            ast_to_source_recursive(s, assignment->left);
            stream_push_const_string(s, "=");
            ast_to_source_recursive(s, assignment->right);
            break;
        }
        case e_binary: {
            Binary* binary=(Binary*)expression;
            ast_to_source_recursive(s, binary->left);
            stream_push_string(s, binary->op);
            ast_to_source_recursive(s, binary->right);
            break;
        }
        case e_conditional: {
            Conditional* conditional=(Conditional*)expression;
            stream_push_const_string(s, "if(");
            ast_to_source_recursive(s, conditional->condition);
            stream_push_const_string(s, ") ");
            ast_to_source_recursive(s, conditional->ontrue);
            if(conditional->onfalse->type!=e_empty){
                if(conditional->onfalse->type==e_conditional){
                    // make elif instead of nested if
                    stream_push_const_string(s, " el");
                    ast_to_source_recursive(s, conditional->onfalse);
                } else {
                    stream_push_const_string(s, " else ");
                    ast_to_source_recursive(s, conditional->onfalse);
                }
            }
            break;
        }
        case e_null_conditional_indexer:
        case e_indexer: {
            Indexer* indexer=(Indexer*)expression;
            ast_to_source_recursive(s, indexer->left);
            if(expression->type==e_null_conditional_indexer){
                stream_push_const_string(s, "?");
            }
            stream_push_const_string(s, "[");
            ast_to_source_recursive(s, indexer->right);
            stream_push_const_string(s, "]");
            break;
        }
        case e_null_conditional_member_access:
        case e_member_access: {
            MemberAccess* member_access=(MemberAccess*)expression;
            ast_to_source_recursive(s, member_access->left);
            if(expression->type==e_null_conditional_member_access){
                stream_push_const_string(s, "?");
            }
            stream_push_const_string(s, ".");
            ast_to_source_recursive(s, (Expression*)member_access->right);
            break;
        }
        case e_block:{
            Block* block=(Block*)expression;
            bool add_parentheses=vector_count(&block->lines)>1;
            if(add_parentheses){
                // TODO: switching between tabbing styles
                stream_push_const_string(s, "{\n    ");
            }
            for(int i=0; i<vector_count(&block->lines); i++){
                if(i!=0){
                    stream_push_const_string(s, "\n    ");
                }
                ast_to_source_recursive(s, pointers_vector_get(&block->lines, i));
            }
            if(add_parentheses){
                stream_push_const_string(s, "\n}");
            }
            break;
        }
        case e_table_literal:{
            Block* block=(Block*)expression;
            stream_push_const_string(s, "[");
            for(int i=0; i<vector_count(&block->lines); i++){
                if(i!=0){
                    stream_push_const_string(s, "\n");
                }
                ast_to_source_recursive(s, pointers_vector_get(&block->lines, i));
            }
            stream_push_const_string(s, "]");
            break;
        }
        case e_function_call:{
            FunctionCall* call=(FunctionCall*)expression;
            ast_to_source_recursive(s, call->called);
            arguments_to_source(s, &call->arguments->lines);
            break;
        }
        case e_message: {
            Message* message=(Message*)expression;
            ast_to_source_recursive(s, message->messaged_object);
            stream_push_const_string(s, "::");
            ast_to_source_recursive(s, (Expression*)message->message_name);
            arguments_to_source(s, &message->arguments->lines);
        }
        case e_function_declaration:{
            FunctionDeclaration* declaration=(FunctionDeclaration*)expression;
            bool add_parentheses=vector_count(&declaration->arguments)>1 || declaration->optional_arguments_count>0;
            if(add_parentheses){
                stream_push_const_string(s, "(");
            }
            for(int i=0; i<vector_count(&declaration->arguments); i++){
                if(i!=0){
                    stream_push_const_string(s, ", ");
                }
                ast_to_source_recursive(s, pointers_vector_get(&declaration->arguments, i));
            }
            if(declaration->variadic){
                stream_push_const_string(s, "...");
            }
            if(add_parentheses){
                stream_push_const_string(s, ")");
            }
            stream_push_const_string(s, "->");
            ast_to_source_recursive(s, declaration->body);
            break;
        }
        default:
            stream_push_const_string(s, "<unhandled>");
    }
}