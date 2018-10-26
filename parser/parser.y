%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vector.h"
#include "ast.h"
#include "parser.h"

// Declare stuff from Flex that Bison needs to know about:
extern int yylex();
extern int yyparse();
extern FILE *yyin;
extern int line_num;

expression* parsing_result;
 
void yyerror(const char *s);

#define YYERROR_VERBOSE 1
%}

// Bison fundamentally works by asking flex to get the next token, which it
// returns as an object of type "yystype".  But tokens could be of any
// arbitrary data type!  So we deal with that in Bison by defining a C union
// holding each of the types of tokens that Flex could return, and have Bison
// use that union instead of "int" for the definition of "yystype":
%union {
	int ival;
	float fval;
	char *sval;
	struct vector* args;
	struct expression* exp;
}

// define the constant-string tokens:
%token ENDL

// define the "terminal symbol" token types I'm going to use (in CAPS
// by convention), and associate each with a field of the union:
%token <ival> INT
%token <fval> FLOAT
%token <sval> STRING
%token <sval> NAME
%token <sval> ASSIGN_UNARY_OPERATOR
%token <sval> UNARY_OPERATOR
%token <sval> PREFIX_OPERATOR
%token <ival> ARROW
%token <ival> IF
%token <ival> ELSE
%token <ival> ELIF

%type <exp> block;
%type <exp> table;
%type <exp> path;
%type <exp> lines;
%type <exp> line;
%type <exp> expression;
%type <exp> literal;
%type <exp> name;
%type <exp> assignment;
%type <exp> unary;
%type <exp> prefix;
%type <exp> call;
%type <args> arguments;
%type <exp> function;
%type <exp> conditional;
%type <exp> conditional_else;

%%
program:
	lines { 
		parsing_result=malloc(sizeof(expression));
		parsing_result = $1;
	}
	;
lines:
	lines line {
		vector_add(&((block*)$1)->lines, $2);
		$$=$1;
	}
	| line	{
		block* b=new_block();
		b->line=line_num;
		vector_init(&b->lines);
		vector_add(&b->lines, $1);
		$$=(expression*)b;
	}
	| expression {
		block* b=new_block();
		b->line=line_num;
		vector_init(&b->lines);
		vector_add(&b->lines, $1);
		$$=(expression*)b;
	}
	;
line:
	expression ENDLS
	| expression ','
	| expression
	;
block:
	OPT_ENDLS '{' OPT_ENDLS lines OPT_ENDLS '}' OPT_ENDLS { $$=$4; }
	;
table:
	OPT_ENDLS '[' OPT_ENDLS lines OPT_ENDLS ']' OPT_ENDLS { 
		$$=$4; 
		$$->type=_table_literal;// by default lines compile to block, so here it's casted to table literal
	}
	;
path:
	name {
		path* p=new_path();
		p->line=line_num;
		vector_init(&p->lines);
		vector_add(&p->lines, $1);
		$$=(expression*)p;
	}
	| path '.' name {
		vector_add(&((path*)$1)->lines, $3);
	}
	| path '[' expression ']' {
		vector_add(&((path*)$1)->lines, $3);
	}
	;
expression:
	'(' expression ')' {$$=$2;}
	| literal
	| block
	| table
	| path
	| assignment
	| call
	| function
	| conditional
	| unary
	| prefix
	;
conditional:
	IF '(' expression ')' expression  {	
		conditional* c=new_conditional();
		c->line=line_num;
		c->condition=$3;
		c->ontrue=$5;
		c->onfalse=(expression*)new_empty();
		$$=(expression*)c;
	}
	| IF '(' expression ')' expression conditional_else {
		conditional* c=new_conditional();
		c->line=line_num;
		c->condition=$3;
		c->ontrue=$5;
		c->onfalse=$6;
		$$=(expression*)c;
	}
	;
conditional_else:
	ELSE expression {
		$$=$2
	}
	| ELIF '(' expression ')' expression conditional_else {
		conditional* c=new_conditional();
		c->line=line_num;
		c->condition=$3;
		c->ontrue=$5;
		c->onfalse=$6;
		$$=(expression*)c;
	}
	| ELIF '(' expression ')' expression {
		conditional* c=new_conditional();
		c->line=line_num;
		c->condition=$3;
		c->ontrue=$5;
		c->onfalse=(expression*)new_empty();
		$$=(expression*)c;
	} 
	;
arguments:
	arguments ',' name {
		vector_add($1, $3);
		$$=$1;
	}
	| name {
		vector* args=malloc(sizeof(vector));
		vector_init(args);
		vector_add(args, $1);
		$$=args;
	}
	;
function:
	'(' arguments ')' ARROW expression {
		function_declaration* f=new_function_declaration();
		f->line=line_num;
		f->arguments=$2;
		f->body=(block*)$5;
		$$=(expression*)f;
	} 
	| name ARROW expression {
		function_declaration* f=new_function_declaration();
		f->line=line_num;
		vector* args=malloc(sizeof(vector));
		vector_init(args);
		vector_add(args, $1);
		f->arguments=args;
		f->body=(block*)$3;
		$$=(expression*)f;
	}
	| ARROW expression {
		function_declaration* f=new_function_declaration();
		vector* args=malloc(sizeof(vector));
		vector_init(args);
		f->line=line_num;
		f->arguments=args;
		f->body=(block*)$2;
		$$=(expression*)f;
	}
	;
literal: 
	INT { 
		literal* l=new_literal();
		l->line=line_num;
		l->ival=$1;
		l->ltype=_int;
		$$=(expression*)l;
	}
	| FLOAT { 
		literal* l=new_literal();
		l->line=line_num;
		l->fval=$1;
		l->ltype=_float;
		$$=(expression*)l;
	}
	| STRING { 
		literal* l=new_literal();
		l->line=line_num;
		l->sval=strdup($1);
		l->ltype=_string;
		$$=(expression*)l;
	}
	;
name:
	NAME {
		name* n=new_name();
		n->line=line_num;
		n->value=malloc(sizeof(char)*100);
		strcpy(n->value, $1);
		$$=(expression*)n;
	  } ;
assignment:
	path ASSIGN_UNARY_OPERATOR expression 
	{
		assignment* a=new_assignment();
		a->line=line_num;
		a->left=(path*)$1;
		unary* u=new_unary();
		u->line=line_num;
		u->left=$1;
		u->op=strdup($2);
		u->right=$3;
		a->right=(expression*)u;
		$$=(expression*)a;
	}
	| path '=' expression 
	{
		assignment* a=new_assignment();
		a->line=line_num;
		a->left=(path*)$1;
		a->right=$3;
		$$=(expression*)a;
	}
	;
call:
	path '(' lines ')' {
		function_call* c=new_function_call();
		c->line=line_num;
		c->function_path=(path*)$1;
		c->arguments=(block*)$3;
		$$=(expression*)c;
	}
	| path '(' ')' {
		function_call* c=new_function_call();
		c->line=line_num;
		c->function_path=(path*)$1;
		c->arguments=new_block();
		vector_init(&c->arguments->lines);
		$$=(expression*)c;
	}
	;
unary:
	expression UNARY_OPERATOR expression 
	{
		unary* u=new_unary();
		u->line=line_num;
		u->left=$1;
		u->op=strdup($2);
		u->right=$3;
		$$=(expression*)u;
	}
	| expression '-' expression 
	{
		unary* u=new_unary();
		u->line=line_num;
		u->left=$1;
		u->op=strdup("-");
		u->right=$3;
		$$=(expression*)u;
	}
	;
prefix:
	'!' expression 
	{
		prefix* p=new_prefix();
		p->line=line_num;
		p->op=strdup("!");
		p->right=$2;
		$$=(expression*)p;
	}
	| '-' expression 
	{
		prefix* p=new_prefix();
		p->line=line_num;
		p->op=strdup("-");
		p->right=$2;
		$$=(expression*)p;
	}
	;
OPT_ENDLS:
	ENDLS
	| ;
ENDLS:
	ENDLS ENDL
	| ENDL ;
%%

void print_and_delete(expression* result){
	printf(stringify_expression(result, 0)); 
	delete_expression(result);
}

typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

void parse_string(char* s) {
	// Set Flex to read from it instead of defaulting to STDIN:
	YY_BUFFER_STATE buffer = yy_scan_string(s);

	// Parse through the input:
	yyparse();
	yy_delete_buffer(buffer);
}

void parse_file(char* file_name) {
	// Open a file handle to a particular file:
	FILE *myfile = fopen(file_name, "r");
	// Make sure it is valid:
	if (!myfile) {
		printf("I can't open the file!\n");
		return;
	}
	// Set Flex to read from it instead of defaulting to STDIN:
	yyin = myfile;
	
	// Parse through the input:
	yyparse();
}

void yyerror(const char *s) {
	printf("Parse error on line %i!  Message: %s\n", line_num, s);
	exit(-1);
}