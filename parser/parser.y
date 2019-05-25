%{
#include "parser.h"
#include "ast_postprocessing.h"

// Declare stuff from Flex that Bison needs to know about:
extern int yylex();
extern int yyparse();
extern FILE *yyin;
extern int line_number;
extern int column_number;
const char* file_name;
bool is_repl;

expression* parsing_result;
 
void yyerror(const char *s);

// if item is already at the end of the vector don't add it again
void vector_add_ignore_duplicate(vector *v, void *item){
    if(vector_last(v)!=item){
        vector_add(v, item);
    }
}

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
%token FOUR_DOTS
%token ELLIPSIS
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
%type <exp> lines_with_return;
%type <exp> lines;
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
%type <exp> message;

%%
program:
	lines { 
		parsing_result = $1;
	}
	| {
		empty* e=new_empty();
		ADD_DEBUG_INFO(e)
		parsing_result=(expression*)e;
	}
	;
lines_separator:
	ENDLS | ',';
lines_separators:
	lines_separator
	| lines_separators lines_separator;
optional_lines_separators:
	lines_separators | ;
lines_with_return:
	lines '!' {
		vector* lines=&((block*)$1)->lines;
		int last_element_index=vector_total(lines)-1;
		function_return* r=new_function_return();
		ADD_DEBUG_INFO(r)
		r->value=vector_get(lines, last_element_index);
		vector_set(lines, last_element_index, r);
		$$=$1;
	}
	;
lines:
	lines lines_separators expression {
		vector_add_ignore_duplicate(&((block*)$1)->lines, $3);
		$$=$1;
	}
	| lines_separators lines {
		$$=$2;
	}
	| lines lines_separators {
		$$=$1;
	}
	| lines_with_return optional_lines_separators expression {
		vector_add_ignore_duplicate(&((block*)$1)->lines, $3);
		$$=$1;
	}
	| expression {
		block* b=new_block();
		ADD_DEBUG_INFO(b)
		vector_init(&b->lines);
		vector_add_ignore_duplicate(&b->lines, $1);
		$$=(expression*)b;
	}
	;
block:
	'{' OPT_ENDLS lines OPT_ENDLS '}' { $$=$3; }
	;
table_contents:
	lines {
		$$=$1; 
		$$->type=e_table_literal;
	}
	;
table:
	'[' OPT_ENDLS table_contents OPT_ENDLS ']' { 
		$$=$3;
	}
	| '[' ']' {
		block* b=new_block();
		ADD_DEBUG_INFO(b)
		vector_init(&b->lines);
		b->type=e_table_literal;
		$$=(expression*)b;
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
	| '(' expression ')' { $$=$2;}
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
	| message
	;
conditional:
	IF '(' expression ')' expression  {	
		conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=$3;
		c->ontrue=$5;
		c->onfalse=(expression*)new_empty();
		$$=(expression*)c;
	}
	| IF '(' expression ')' expression conditional_else {
		conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
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
		ADD_DEBUG_INFO(c)
		c->condition=$3;
		c->ontrue=$5;
		c->onfalse=$6;
		$$=(expression*)c;
	}
	| ELIF '(' expression ')' expression {
		conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
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
		f->variadic=false;
		f->body=$5;
		$$=(expression*)f;
	}
	| '(' arguments ELLIPSIS ')' ARROW expression {
		function_declaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		f->arguments=$2;
		f->variadic=true;
		f->body=$6;
		$$=(expression*)f;
	}
	| name ELLIPSIS ARROW expression {
		function_declaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		vector* args=malloc(sizeof(vector));
		CHECK_ALLOCATION(args);
		vector_init(args);
		vector_add(args, $1);
		f->arguments=args;
		f->variadic=true;
		f->body=$4;
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
		f->variadic=false;
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
		f->variadic=false;
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
		a->used_in_closure=false;
		$$=(expression*)a;
	}
	| path '=' expression 
	{
		assignment* a=new_assignment();
		ADD_DEBUG_INFO(a)
		a->left=(path*)$1;
		a->right=$3;
		a->used_in_closure=false;
		$$=(expression*)a;
	}
	;
call:
	expression '(' lines ')' {
		function_call* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->called=$1;
		c->arguments=(table_literal*)$3;
		c->arguments->type=e_table_literal;
		$$=(expression*)c;
	}
	| expression '(' ')' {
		function_call* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->called=$1;
		c->arguments=(table_literal*)new_block();
		vector_init(&c->arguments->lines);
		$$=(expression*)c;
	}
	;
message:
	expression FOUR_DOTS name '(' lines ')' {
		message* m=new_message();
		ADD_DEBUG_INFO(m)
		m->messaged_object=$1;
		m->message_name=$3;
		m->arguments=(table_literal*)$5;
		$$=(expression*)m;
	}
	| expression FOUR_DOTS name '(' ')' {
		message* m=new_message();
		ADD_DEBUG_INFO(m)
		m->messaged_object=$1;
		m->message_name=$3;
		m->arguments=(table_literal*)new_block();
		$$=(expression*)m;
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
extern void reset_lexer(void);

expression* parse_string(const char* s) {
	line_number=1;
	column_number=0;
	file_name="repl";

	reset_lexer();

	// Set Flex to read from it instead of defaulting to STDIN:
	YY_BUFFER_STATE buffer = yy_scan_string(s);

	// Parse through the input:
	yyparse();
	yy_delete_buffer(buffer);
	postprocess_ast(parsing_result);
	return parsing_result;
}

expression* parse_file(const char* file) {
	line_number=1;
	column_number=0;
	file_name=file;

	reset_lexer();

	// Open a file handle to a particular file:
	FILE *myfile = fopen(file, "r");
	if (!myfile) {
		printf("I can't open the file!\n");
		return NULL;
	}
	// Set Flex to read from it instead of defaulting to STDIN:
	yyin = myfile;
	
	// Parse through the input:
	yyparse();
	postprocess_ast(parsing_result);
	return parsing_result;
}

char* get_line(const char* file, int line){
	size_t size = 32;
    char *buf = malloc(size+1);

    FILE* f = fopen(file, "r");
    if (f == NULL) {
        return strdup("");
    }

	int l=1;
    while ((getline(&buf, &size, f)) != -1) {
		if(l==line){
			fclose(f);
			return buf;
		}
        line++;
    }
	return strdup("");
}

void print_arrow(int length){
	printf("%.*s^\n", length, "----------------------------------------------------------------------");
}

void yyerror(const char *message) {
	bool is_repl=strcmp(file_name, "repl")==0;
	if(is_repl){
		print_arrow(column_number-1);
	}
	printf("ParsingError: %s\nat(%s:%i:%i)\n", message, file_name, line_number, column_number);
	if(!is_repl){
		char * line=get_line(file_name, line_number);
		printf("%s\n", line);
		print_arrow(column_number-1);
		free(line);
	}
	parsing_result=NULL;
}