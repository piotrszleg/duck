snazzle.l
%{
#include <string.h>
#include <stdio.h>
#include "error.h"
#include "parser.tab.h"
int line_num = 1;

// removes first and last character from source
char* unquote(const char* source){
	int length=strlen(source)-1;
	char* result=malloc(length);
	CHECK_ALLOCATION(result);
	strncpy(result, source+1, length);
	result[length-1]='\0';
	return result;
}

// removes last character from source
char* remove_last_character(const char* source){
	int length=strlen(source);
	char* result=malloc(length);
	CHECK_ALLOCATION(result);
	strncpy(result, source, length);
	result[length-1]='\0';
	return result;
}

%}
%option noyywrap
%s COMMENT
%%
[ \t]               ;
\n                  { ++line_num; return ENDL; }
#.*                 ;// comment
if                  { return IF; }
elif                { return ELIF; }
else                { return ELSE; }
null				{ return NULL_LITERAL; }
[0-9]+\.[0-9]+      { yylval.fval = atof(yytext); return FLOAT; }
[0-9]+              { yylval.ival = atoi(yytext); return INT; }
\"([^\\\"]|\\.)*\"    { yylval.sval = unquote(yytext); return STRING; }// god bless stack overflow
[a-zA-Z_][a-zA-Z0-9_]*        { yylval.sval = strdup(yytext); return NAME; }
->                  { return ARROW; }
!=					{ yylval.sval = strdup(yytext); return UNARY_OPERATOR; }
==					{ yylval.sval = strdup(yytext); return UNARY_OPERATOR; }
[\(\)\{\},\.#=\[\]!-]   { return yytext[0]; }// symbols used directly by parser, - must be separated from other unary operators because it is also a prefix
[+\-\/*^&|<>]+      { yylval.sval = strdup(yytext); return UNARY_OPERATOR; }
[+\-\/*^&|]+=       { yylval.sval = remove_last_character(yytext); return ASSIGN_UNARY_OPERATOR; }// +=, -= etc
`.+?`             	{ yylval.sval = unquote(yytext); return UNARY_OPERATOR; }// function names can be used as operators, for example: [1,2,3] `map` timestwo
%%