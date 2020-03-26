%{
#include "parser.h"
#include "ast/ast_postprocessing.h"

// Declare stuff from Flex that Bison needs to know about:
extern int yylex();
extern int yyparse();
extern FILE *yyin;
extern int line_number;
extern int column_number;
const char* file_name;
bool is_repl;

Expression* parsing_result;
 
void yyerror(const char *s);

// if item is already at the end of the vector don't add it again
void pointers_vector_push_ignore_duplicate(vector *v, void *item){
    if(vector_top(v)!=item){
        pointers_vector_push(v, item);
    }
}

#define ADD_DEBUG_INFO(expression) expression->line_number=line_number; expression->column_number=column_number;

Argument* name_to_argument(Name* n) {
	Argument* a=new_argument();
	ADD_DEBUG_INFO(a)
	a->name=strdup(n->value);
	a->used_in_closure=false;
	delete_expression((Expression*)n);
	return a;
}

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
	struct Expression* expression;
}

// define the constant-string tokens:
%token ENDL
%token IF
%token ELIF
%token ELSE
%token NULL_LITERAL
%token DOT
%token ELLIPSIS
%token ARROW
%token FOUR_DOTS
%token AT
%token QUESTION_MARK 

// define the "terminal symbol" token types and associate each with a field of the union:
%token <ival> INT
%token <fval> FLOAT
%token <sval> STRING
%token <sval> NAME
%token <sval> ASSIGN_BINARY_OPERATOR
%token <sval> BINARY_OPERATOR
%token <sval> PREFIX_OPERATOR

%type <expression> block;
%type <expression> table;
%type <expression> table_contents;
%type <expression> identifier;
%type <expression> member_access;
%type <expression> self_member_access;
%type <expression> null_conditional_member_access;
%type <expression> indexer;
%type <expression> null_conditional_indexer;
%type <expression> self_indexer;
%type <expression> lines;
%type <expression> expression;
%type <expression> literal;
%type <expression> null_literal;
%type <expression> name;
%type <expression> assignment;
%type <expression> binary;
%type <expression> prefix;
%type <expression> call;
%type <args> arguments;
%type <expression> function;
%type <expression> conditional;
%type <expression> conditional_else;
%type <expression> message;
%type <expression> parentheses;
%type <expression> macro;
%type <expression> macro_declaration;
%type <expression> argument;
%type <expression> return;
%type <expression> return_if_error;

%%
program:
	lines { 
		parsing_result = $1;
	}
	| {
		Empty* e=new_empty();
		ADD_DEBUG_INFO(e)
		parsing_result=(Expression*)e;
	}
	;
lines_separator:
	ENDLS | ',';
lines_separators:
	lines_separator
	| lines_separators lines_separator;
lines:
	lines lines_separators expression {
		pointers_vector_push_ignore_duplicate(&((Block*)$1)->lines, $3);
		$$=$1;
	}
	| lines_separators lines {
		$$=$2;
	}
	| lines lines_separators {
		$$=$1;
	}
	| expression {
		Block* b=new_block();
		ADD_DEBUG_INFO(b)
		pointers_vector_push_ignore_duplicate(&b->lines, $1);
		$$=(Expression*)b;
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
		Block* b=new_block();
		ADD_DEBUG_INFO(b)
		b->type=e_table_literal;
		$$=(Expression*)b;
	}
	;
expression:
	| parentheses
	| literal
	| block
	| table
	| identifier
	| assignment
	| call
	| function
	| conditional
	| binary
	| prefix
	| null_literal
	| message
	| macro
	| macro_declaration
	| return
	| return_if_error
	;
parentheses:
	'(' expression ')' {
		Parentheses* p=new_parentheses();
		ADD_DEBUG_INFO(p)
		p->value=$2;
		$$=(Expression*)p;
	}
	;
identifier:
	name
	| self_member_access
	| member_access
	| self_indexer
	| indexer
	| null_conditional_member_access
	| null_conditional_indexer
	;
self_member_access:
	DOT name {
		SelfMemberAccess* ma=new_self_member_access();
		ADD_DEBUG_INFO(ma)
		ma->right=(Name*)$2;
		$$=(Expression*)ma;
	}
	;
member_access:
	expression DOT name {
		MemberAccess* ma=new_member_access();
		ADD_DEBUG_INFO(ma)
		ma->left=$1;
		ma->right=(Name*)$3;
		$$=(Expression*)ma;
	}
	;
null_conditional_member_access:
	expression QUESTION_MARK DOT name {
		NullConditionalMemberAccess* ma=new_null_conditional_member_access();
		ADD_DEBUG_INFO(ma)
		ma->left=$1;
		ma->right=(Name*)$4;
		$$=(Expression*)ma;
	}
	;
indexer:
	expression '[' expression ']' {
		Indexer* i=new_indexer();
		ADD_DEBUG_INFO(i)
		i->left=$1;
		i->right=$3;
		$$=(Expression*)i;
	}
	;
null_conditional_indexer:
	expression QUESTION_MARK '[' expression ']' {
		NullConditionalIndexer* i=new_null_conditional_indexer();
		ADD_DEBUG_INFO(i)
		i->left=$1;
		i->right=$4;
		$$=(Expression*)i;
	}
	;
self_indexer:
	DOT '[' expression ']' {
		SelfIndexer* i=new_self_indexer();
		ADD_DEBUG_INFO(i)
		i->right=$3;
		$$=(Expression*)i;
	}
	;
return:
	expression '!' {
		FunctionReturn* r=new_function_return();
		ADD_DEBUG_INFO(r)
		r->value=$1;
		$$=(Expression*)r;
	}
	;
macro:
	AT name {
		Macro* m=new_macro();
		ADD_DEBUG_INFO(m)
		m->identifier=(Name*)$2;
		$$=(Expression*)m;
	}
	;
conditional:
	IF '(' expression ')' expression  {	
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=$3;
		c->ontrue=$5;
		c->onfalse=(Expression*)new_empty();
		$$=(Expression*)c;
	}
	| IF '(' expression ')' expression conditional_else {
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=$3;
		c->ontrue=$5;
		c->onfalse=$6;
		$$=(Expression*)c;
	}
	;
conditional_else:
	ELSE expression {
		$$=$2;
	}
	| ELIF '(' expression ')' expression conditional_else {
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=$3;
		c->ontrue=$5;
		c->onfalse=$6;
		$$=(Expression*)c;
	}
	| ELIF '(' expression ')' expression {
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=$3;
		c->ontrue=$5;
		c->onfalse=(Expression*)new_empty();
		$$=(Expression*)c;
	} 
	;
arguments:
	arguments ',' argument {
		pointers_vector_push($1, $3);
		$$=$1;
	}
	| argument {
		vector* args=malloc(sizeof(vector));
		CHECK_ALLOCATION(args);
		vector_init(args, sizeof(Expression*), 4);
		pointers_vector_push(args, $1);
		$$=args;
	}
	;
argument:
	NAME {
		Argument* a=new_argument();
		ADD_DEBUG_INFO(a)
		a->name=$1;
		a->used_in_closure=false;
		$$=(Expression*)a;
	}
	;
function:
	'(' arguments ')' ARROW expression {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		vector_copy($2, &f->arguments);
		vector_deinit($2);
		free($2);
		f->variadic=false;
		f->body=$5;
		$$=(Expression*)f;
	}
	| '(' arguments ELLIPSIS ')' ARROW expression {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		vector_copy($2, &f->arguments);
		vector_deinit($2);
		free($2);
		f->variadic=true;
		f->body=$6;
		$$=(Expression*)f;
	}
	| name ELLIPSIS ARROW expression {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		pointers_vector_push(&f->arguments, name_to_argument((Name*)$1));
		f->variadic=true;
		f->body=$4;
		$$=(Expression*)f;
	}
	| name ARROW expression {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		pointers_vector_push(&f->arguments, name_to_argument((Name*)$1));
		f->variadic=false;
		f->body=$3;
		$$=(Expression*)f;
	}
	| ARROW expression {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		f->variadic=false;
		f->body=$2;
		$$=(Expression*)f;
	}
	;
literal: 
	INT {
		IntLiteral* l=new_int_literal();
		ADD_DEBUG_INFO(l)
		l->value=$1;
		$$=(Expression*)l;
	}
	| FLOAT { 
		FloatLiteral* l=new_float_literal();
		ADD_DEBUG_INFO(l)
		l->value=$1;
		$$=(Expression*)l;
	}
	| STRING { 
		StringLiteral* l=new_string_literal();
		ADD_DEBUG_INFO(l)
		l->value=$1;
		$$=(Expression*)l;
	}
	;
null_literal:
	NULL_LITERAL {
		NullLiteral* n=new_null_literal();
		ADD_DEBUG_INFO(n)
		$$=(Expression*)n;
	}
	;
name:
	NAME {
		Name* n=new_name();
		ADD_DEBUG_INFO(n)
		n->value=$1;
		$$=(Expression*)n;
	  }
	  ;
assignment:
	identifier ASSIGN_BINARY_OPERATOR expression 
	{
		Assignment* a=new_assignment();
		ADD_DEBUG_INFO(a)
		a->left=(Expression*)$1;
		Binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=copy_expression($1);
		u->op=$2;
		u->right=$3;
		a->right=(Expression*)u;
		a->used_in_closure=false;
		$$=(Expression*)a;
	}
	| identifier '=' expression 
	{
		Assignment* a=new_assignment();
		ADD_DEBUG_INFO(a)
		a->left=(Expression*)$1;
		a->right=$3;
		a->used_in_closure=false;
		$$=(Expression*)a;
	}
	;
macro_declaration:
	macro '=' expression 
	{
		MacroDeclaration* md=new_macro_declaration();
		ADD_DEBUG_INFO(md)
		md->left=(Macro*)$1;
		md->right=$3;
		$$=(Expression*)md;
	}
	;
call:
	expression '(' lines ')' {
		FunctionCall* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->called=$1;
		c->arguments=(TableLiteral*)$3;
		c->arguments->type=e_table_literal;
		$$=(Expression*)c;
	}
	| expression '(' ')' {
		FunctionCall* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->called=$1;
		c->arguments=(TableLiteral*)new_block();
		$$=(Expression*)c;
	}
	;
message:
	expression FOUR_DOTS name '(' lines ')' {
		Message* m=new_message();
		ADD_DEBUG_INFO(m)
		m->messaged_object=$1;
		m->message_name=(Name*)$3;
		m->arguments=(TableLiteral*)$5;
		m->arguments->type=e_table_literal;
		$$=(Expression*)m;
	}
	| expression FOUR_DOTS name '(' ')' {
		Message* m=new_message();
		ADD_DEBUG_INFO(m)
		m->messaged_object=$1;
		m->message_name=(Name*)$3;
		m->arguments=(TableLiteral*)new_block();
		$$=(Expression*)m;
	}
	;
binary:
	expression BINARY_OPERATOR expression 
	{
		Binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=$1;
		u->op=$2;
		u->right=$3;
		$$=(Expression*)u;
	}
	| expression '-' expression 
	{
		Binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=$1;
		u->op=strdup("-");
		u->right=$3;
		$$=(Expression*)u;
	}
	;
prefix:
	'!' expression 
	{
		Prefix* p=new_prefix();
		ADD_DEBUG_INFO(p)
		p->op=strdup("!");
		p->right=$2;
		$$=(Expression*)p;
	}
	| '-' expression 
	{
		Prefix* p=new_prefix();
		ADD_DEBUG_INFO(p)
		p->op=strdup("-");
		p->right=$2;
		$$=(Expression*)p;
	}
	;
return_if_error:
	expression '!' QUESTION_MARK
	{
		ReturnIfError* r=new_return_if_error();
		ADD_DEBUG_INFO(r)
		r->value=$1;
		$$=(Expression*)r;
	}
	;
OPT_ENDLS:
	ENDLS
	| ;
ENDLS:
	ENDLS ENDL
	| ENDL ;
%%

void print_and_delete(Expression* result){
	printf("%s", stringify_expression(result, 0)); 
	delete_expression(result);
}

typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(const char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern void reset_lexer(void);

Expression* parse_string(const char* s) {
	line_number=1;
	column_number=0;
	file_name="repl";

	reset_lexer();

	// Set Flex to read from it instead of defaulting to STDIN:
	YY_BUFFER_STATE buffer = yy_scan_string(s);

	// Parse through the input:
	yyparse();
	yy_delete_buffer(buffer);
	if(parsing_result!=NULL){
		postprocess_ast(&parsing_result);
	}
	return parsing_result;
}

Expression* parse_file(const char* file) {
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
	if(parsing_result!=NULL){
		postprocess_ast(&parsing_result);
	}
	return parsing_result;
}

char* get_source_line(const char* file, int line){
	size_t size = 128;
    char *buf = malloc(size+1);

    FILE* f = fopen(file, "r");
    if (f == NULL) {
        return strdup("");
    }

	int l=1;
    while (fgets_no_newline(buf, size, f) != NULL) {
		if(l==line){
			fclose(f);
			return buf;
		}
        l++;
    }
	fclose(f);
	return strdup("");
}

void print_arrow(int length){
	for(int i=0; i<length; i++){
		printf("-");
	}
	printf("^\n");
}

void yyerror(const char *message) {
	bool is_repl=strcmp(file_name, "repl")==0;
	if(is_repl){
		// two arrow needs to be two characters longer than normal because of ">>" prompt text
		print_arrow(column_number+1);
	}
	printf("ParsingError: %s\nat(%s:%i:%i)\n", message, file_name, line_number, column_number);
	if(!is_repl){
		char* line=get_source_line(file_name, line_number);
		printf("%s\n", line);
		print_arrow(column_number-1);
		free(line);
	}
	parsing_result=NULL;
}