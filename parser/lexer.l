snazzle.l
%{
#include <string.h>
#include <stdio.h>
#include "parser.tab.h"
int line_num = 1;

// removes first and last character from source
char* unquote(char* source){
	int length=strlen(source)-2;
	char* result=malloc(length);
	strncpy(result, source+1, length);
	result[length]='\0';
	return result;
}

// removes last character from source
char* remove_last_character(char* source){
	int length=strlen(source)-1;
	char* result=malloc(length);
	strncpy(result, source, length);
	result[length]='\0';
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
[0-9]+\.[0-9]+      { yylval.fval = atof(yytext); return FLOAT; }
[0-9]+              { yylval.ival = atoi(yytext); return INT; }
\"[a-zA-Z0-9\. ]+\"    { yylval.sval = unquote(yytext); return STRING; }
[a-zA-Z0-9]+        { yylval.sval = strdup(yytext); return NAME; }
->                  { return ARROW; }
!=					{ yylval.sval = strdup(yytext); return UNARY_OPERATOR; }
==					{ yylval.sval = strdup(yytext); return UNARY_OPERATOR; }
[\(\)\{\},\.#=\[\]]   { return yytext[0]; }// symbols used directly by parser
[!-]        		{ yylval.sval = strdup(yytext); return PREFIX_OPERATOR; }
[+\-\/*^&|<>]+      { yylval.sval = strdup(yytext); return UNARY_OPERATOR; }
[+\-\/*^&|]+=       { yylval.sval = remove_last_character(yytext); return ASSIGN_UNARY_OPERATOR; }// +=, -= etc
`.+?`             	{ yylval.sval = unquote(yytext); return UNARY_OPERATOR; }// function names can be used as operators, for example: [1,2,3] `map` timestwo

.                   ;
%%