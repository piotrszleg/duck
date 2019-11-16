#ifndef AST_H
#define AST_H

#include <string.h>
#include "../containers/vector.h"
#include "../containers/stream.h"
#include "../error/error.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef COUNT_AST_ALLOCATIONS
bool ast_allocations_zero();
#endif

#define AST_EXPRESSIONS \
    EXPRESSION(Expression, expression) \
    END \
    EXPRESSION(Empty, empty) \
    END \
    EXPRESSION(NullLiteral, null_literal) \
    END \
    EXPRESSION(Block, block) \
        VECTOR_FIELD(lines)\
    END \
    EXPRESSION(TableLiteral, table_literal) \
        VECTOR_FIELD(lines)\
    END \
    EXPRESSION(Name, name) \
        STRING_FIELD(value) \
    END \
    EXPRESSION(SelfMemberAccess, self_member_access) \
        SPECIFIED_EXPRESSION_FIELD(Name, name, right) \
    END \
    EXPRESSION(MemberAccess, member_access) \
        EXPRESSION_FIELD(left) \
        SPECIFIED_EXPRESSION_FIELD(Name, name, right) \
    END \
    EXPRESSION(Indexer, indexer) \
        EXPRESSION_FIELD(left) \
        EXPRESSION_FIELD(right) \
    END \
    EXPRESSION(SelfIndexer, self_indexer) \
        EXPRESSION_FIELD(right) \
    END \
    EXPRESSION(NullConditionalMemberAccess, null_conditional_member_access) \
        EXPRESSION_FIELD(left) \
        SPECIFIED_EXPRESSION_FIELD(Name, name, right) \
    END \
    EXPRESSION(NullConditionalIndexer, null_conditional_indexer) \
        EXPRESSION_FIELD(left) \
        EXPRESSION_FIELD(right) \
    END \
    EXPRESSION(Assignment, assignment) \
        EXPRESSION_FIELD(left)\
        EXPRESSION_FIELD(right) \
        BOOL_FIELD(used_in_closure) \
    END \
    EXPRESSION(FunctionCall, function_call) \
        EXPRESSION_FIELD(called) \
        SPECIFIED_EXPRESSION_FIELD(TableLiteral, table_literal, arguments) \
    END \
    EXPRESSION(Binary, binary) \
        STRING_FIELD(op) \
        EXPRESSION_FIELD(left)\
        EXPRESSION_FIELD(right) \
    END \
    EXPRESSION(Prefix, prefix) \
        STRING_FIELD(op) \
        EXPRESSION_FIELD(right) \
    END \
    EXPRESSION(FunctionDeclaration, function_declaration) \
        VECTOR_FIELD(arguments) \
        BOOL_FIELD(variadic) \
        EXPRESSION_FIELD(body) \
    END \
    EXPRESSION(Conditional, conditional) \
        EXPRESSION_FIELD(condition) \
        EXPRESSION_FIELD(ontrue) \
        EXPRESSION_FIELD(onfalse) \
    END \
    EXPRESSION(FunctionReturn, function_return) \
        EXPRESSION_FIELD(value) \
    END \
    EXPRESSION(Parentheses, parentheses) \
        EXPRESSION_FIELD(value) \
    END \
    EXPRESSION(ReturnIfError, return_if_error) \
        EXPRESSION_FIELD(value) \
    END \
    EXPRESSION(Message, message) \
        EXPRESSION_FIELD(messaged_object) \
        SPECIFIED_EXPRESSION_FIELD(Name, name, message_name) \
        SPECIFIED_EXPRESSION_FIELD(TableLiteral, table_literal, arguments) \
    END \
    EXPRESSION(Macro, macro) \
        SPECIFIED_EXPRESSION_FIELD(Name, name, identifier) \
    END \
    EXPRESSION(MacroDeclaration, macro_declaration) \
        SPECIFIED_EXPRESSION_FIELD(Macro, macro, left) \
        EXPRESSION_FIELD(right) \
    END \
    EXPRESSION(IntLiteral, int_literal) \
        INT_FIELD(value) \
    END \
    EXPRESSION(FloatLiteral, float_literal) \
        FLOAT_FIELD(value) \
    END \
    EXPRESSION(StringLiteral, string_literal) \
        STRING_FIELD(value) \
    END \
    EXPRESSION(Argument, argument) \
        STRING_FIELD(name) \
        BOOL_FIELD(used_in_closure) \
    END

#define EXPRESSION_TYPES_COUNT (int)e_argument

// generate enum of expression types, each value is prepended with "e_"
typedef enum expression_type expression_type;
enum expression_type{
    #define EXPRESSION(struct_name, type_tag) e_##type_tag,
    #define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name)
    #define EXPRESSION_FIELD(field_name)
    #define BOOL_FIELD(field_name)    
    #define STRING_FIELD(field_name)
    #define VECTOR_FIELD(field_name)
    #define INT_FIELD(field_name)
    #define FLOAT_FIELD(field_name)
    #define END 

    AST_EXPRESSIONS

    #undef EXPRESSION
    #undef SPECIFIED_EXPRESSION_FIELD
    #undef EXPRESSION_FIELD               
    #undef BOOL_FIELD                   
    #undef STRING_FIELD
    #undef VECTOR_FIELD
    #undef FLOAT_FIELD
    #undef INT_FIELD
    #undef END
};

typedef enum literal_type literal_type;
enum literal_type{ l_int, l_float, l_string };

// struct declarations and new_<expression> functions
#define EXPRESSION(struct_name, type_tag) \
    typedef struct struct_name struct_name; \
    struct_name* new_ ## type_tag(); \
    struct struct_name { \
        expression_type type; \
        int line_number; \
        int column_number; \

#define SPECIFIED_EXPRESSION_FIELD(struct_name, type_tag, field_name) \
                                                    struct_name* field_name;
#define EXPRESSION_FIELD(field_name)                Expression* field_name;
#define BOOL_FIELD(field_name)                      bool field_name;
#define STRING_FIELD(field_name)                    char* field_name;
#define VECTOR_FIELD(field_name)                    vector field_name;
#define INT_FIELD(field_name)                       int field_name;
#define FLOAT_FIELD(field_name)                     float field_name;
#define END \
    };

AST_EXPRESSIONS

#undef EXPRESSION
#undef SPECIFIED_EXPRESSION_FIELD
#undef EXPRESSION_FIELD               
#undef BOOL_FIELD                   
#undef STRING_FIELD
#undef VECTOR_FIELD
#undef FLOAT_FIELD
#undef INT_FIELD
#undef END

bool check_expression(Expression* e);
char* stringify_expression(Expression*, int);
void delete_expression_keep_children(Expression* expression);
void delete_expression(Expression*);
Expression* copy_expression(Expression*);
bool expressions_equal(Expression* expression_a, Expression* expression_b);
void allow_unused_variable(void* variable);

#endif