%{
#include "parser.h"

// Declare stuff from Flex that Bison needs to know about:
extern int yylex();
extern int yyparse();
extern FILE *yyin;
extern int line_number;
extern int column_number;

expression* parsing_result;
 
void yyerror(const char *s);

#define ADD_DEBUG_INFO(exp) exp->line_number=line_number; exp->column_number=column_number;

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
%token ARROW
%token IF
%token ELSE
%token ELIF
%token NULL_LITERAL

// define the "terminal symbol" token types I'm going to use (in CAPS
// by convention), and associate each with a field of the union:
%token <ival> INT
%token <fval> FLOAT
%token <sval> STRING
%token <sval> NAME
%token <sval> ASSIGN_UNARY_OPERATOR
%token <sval> UNARY_OPERATOR
%token <sval> PREFIX_OPERATOR

%type <exp> block;
%type <exp> table;
%type <exp> table_contents;
%type <exp> path;
%type <exp> lines;
%type <exp> line;
%type <exp> expression;
%type <exp> literal;
%type <exp> null;
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
		ADD_DEBUG_INFO(b)
		vector_init(&b->lines);
		vector_add(&b->lines, $1);
		$$=(expression*)b;
	}
	| expression {
		block* b=new_block();
		ADD_DEBUG_INFO(b)
		vector_init(&b->lines);
		vector_add(&b->lines, $1);
		$$=(expression*)b;
	}
	;
line:
	expression ENDLS
	| expression ','
	| expression
	| expression '!' OPT_ENDLS
	{
		function_return* r=new_function_return();
		ADD_DEBUG_INFO(r)
		r->value=$1;
		$$=(expression*)r;
	}
	;
block:
	OPT_ENDLS '{' OPT_ENDLS lines OPT_ENDLS '}' OPT_ENDLS { $$=$4; }
	;
table_contents:
	lines {
		$$=$1; 
		$$->type=e_table_literal;
	}
	| {
		block* b=new_block();
		ADD_DEBUG_INFO(b)
		vector_init(&b->lines);
		b->type=e_table_literal;
		$$=(expression*)b;
	}
	;
table:
	OPT_ENDLS '[' OPT_ENDLS table_contents OPT_ENDLS ']' OPT_ENDLS { 
		$$=$4;
	}
	;
path:
	name {
		path* p=new_path();
		ADD_DEBUG_INFO(p)
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
	| null
	;
conditional:
	IF '(' expression ')' OPT_ENDLS line  {	
		conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=$3;
		c->ontrue=$6;
		c->onfalse=(expression*)new_empty();
		$$=(expression*)c;
	}
	| IF '(' expression ')' OPT_ENDLS line conditional_else {
		conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=$3;
		c->ontrue=$6;
		c->onfalse=$7;
		$$=(expression*)c;
	}
	;
conditional_else:
	ELSE expression {
		$$=$2
	}
	| ELIF '(' expression ')' OPT_ENDLS line conditional_else {
		conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=$3;
		c->ontrue=$6;
		c->onfalse=$7;
		$$=(expression*)c;
	}
	| ELIF '(' expression ')' OPT_ENDLS line {
		conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=$3;
		c->ontrue=$6;
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
		CHECK_ALLOCATION(args);
		vector_init(args);
		vector_add(args, $1);
		$$=args;
	}
	;
function:
	'(' arguments ')' ARROW expression {
		function_declaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		f->arguments=$2;
		f->body=$5;
		$$=(expression*)f;
	} 
	| '(' name ')' ARROW expression {
		function_declaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		vector* args=malloc(sizeof(vector));
		CHECK_ALLOCATION(args);
		vector_init(args);
		vector_add(args, $2);
		f->arguments=args;
		f->body=$5;
		$$=(expression*)f;
	}
	| name ARROW expression {
		function_declaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		vector* args=malloc(sizeof(vector));
		CHECK_ALLOCATION(args);
		vector_init(args);
		vector_add(args, $1);
		f->arguments=args;
		f->body=$3;
		$$=(expression*)f;
	}
	| ARROW expression {
		function_declaration* f=new_function_declaration();
		vector* args=malloc(sizeof(vector));
		CHECK_ALLOCATION(args);
		vector_init(args);
		ADD_DEBUG_INFO(f)
		f->arguments=args;
		f->body=$2;
		$$=(expression*)f;
	}
	;
literal: 
	INT { 
		literal* l=new_literal();
		ADD_DEBUG_INFO(l)
		l->ival=$1;
		l->ltype=l_int;
		$$=(expression*)l;
	}
	| FLOAT { 
		literal* l=new_literal();
		ADD_DEBUG_INFO(l)
		l->fval=$1;
		l->ltype=l_float;
		$$=(expression*)l;
	}
	| STRING { 
		literal* l=new_literal();
		ADD_DEBUG_INFO(l)
		l->sval=$1;
		l->ltype=l_string;
		$$=(expression*)l;
	}
	;
null:
	NULL_LITERAL {
		empty* e=new_empty();
		ADD_DEBUG_INFO(e)
		$$=(expression*)e;
	}
	;
name:
	NAME {
		name* n=new_name();
		ADD_DEBUG_INFO(n)
		n->value=$1;
		$$=(expression*)n;
	  }
	  ;
assignment:
	path ASSIGN_UNARY_OPERATOR expression 
	{
		assignment* a=new_assignment();
		ADD_DEBUG_INFO(a)
		a->left=(path*)$1;
		unary* u=new_unary();
		ADD_DEBUG_INFO(u)
		u->left=$1;
		u->op=$2;
		u->right=$3;
		a->right=(expression*)u;
		$$=(expression*)a;
	}
	| path '=' expression 
	{
		assignment* a=new_assignment();
		ADD_DEBUG_INFO(a)
		a->left=(path*)$1;
		a->right=$3;
		$$=(expression*)a;
	}
	;
call:
	path '(' lines ')' {
		function_call* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->function_path=(path*)$1;
		c->arguments=(table_literal*)$3;
		c->arguments->type=e_table_literal;
		$$=(expression*)c;
	}
	| path '(' ')' {
		function_call* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->function_path=(path*)$1;
		c->arguments=(table_literal*)new_block();
		vector_init(&c->arguments->lines);
		$$=(expression*)c;
	}
	;
unary:
	expression UNARY_OPERATOR expression 
	{
		unary* u=new_unary();
		ADD_DEBUG_INFO(u)
		u->left=$1;
		u->op=$2;
		u->right=$3;
		$$=(expression*)u;
	}
	| expression '-' expression 
	{
		unary* u=new_unary();
		ADD_DEBUG_INFO(u)
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
		ADD_DEBUG_INFO(p)
		p->op=strdup("!");
		p->right=$2;
		$$=(expression*)p;
	}
	| '-' expression 
	{
		prefix* p=new_prefix();
		ADD_DEBUG_INFO(p)
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
extern YY_BUFFER_STATE yy_scan_string(const char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

void parse_string(const char* s) {
	line_number=0;
	column_number=0;

	// Set Flex to read from it instead of defaulting to STDIN:
	YY_BUFFER_STATE buffer = yy_scan_string(s);

	// Parse through the input:
	yyparse();
	yy_delete_buffer(buffer);
}

void parse_file(const char* file_name) {
	line_number=0;
	column_number=0;

	// Open a file handle to a particular file:
	FILE *myfile = fopen(file_name, "r");
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
	printf("Parse error on line %i, column %i\nMessage: %s\n", line_number, column_number, s);
	exit(-1);
}