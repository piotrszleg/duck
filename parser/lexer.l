lexer.l
%{
#include <string.h>
#include <stdio.h>
#include "../error/error.h"
#include "../utility.h"
#include "parser.tab.h"
int line_number = 1;
int column_number = 0;

// removes first and last character from source
char* proccess_quoted_string(const char* source){
	int length=strlen(source)-1;
	char* quote_removed=malloc(length);
	CHECK_ALLOCATION(quote_removed);
	strncpy(quote_removed, source+1, length);
	quote_removed[length-1]='\0';

	ReplacementPair replacements[]={
        CONSTANT_REPLACEMENT_PAIR("\\\"", "\""),
        CONSTANT_REPLACEMENT_PAIR("\\t", "\t"),
		CONSTANT_REPLACEMENT_PAIR("\\n", "\n"),
		CONSTANT_REPLACEMENT_PAIR("\\\\", "\\")
    };
    char* result=string_replace_multiple(quote_removed, replacements, 3);
	free(quote_removed);
	return result;
}

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
	for(int i=0; i<length-1; i++){
		result[i]=source[i];
	}
	result[length-1]='\0';
	return result;
}

void reset_lexer(){
	YY_FLUSH_BUFFER;
	BEGIN(0);
}

#define COUNT_COLUMNS column_number+=strlen(yytext)

%}
%option noyywrap
%s COMMENT
%%
[ \t]                   { COUNT_COLUMNS; }
\n                      { line_number++; column_number=0; return ENDL; }
#.*                     ;// comment
if                      { COUNT_COLUMNS; return IF; }
elif                    { COUNT_COLUMNS; return ELIF; }
else                    { COUNT_COLUMNS; return ELSE; }
null				    { COUNT_COLUMNS; return NULL_LITERAL; }
@						{ COUNT_COLUMNS; return AT; }	
\.\.\.					{ COUNT_COLUMNS; return ELLIPSIS; }
->                      { COUNT_COLUMNS; return ARROW; }
::                      { COUNT_COLUMNS; return FOUR_DOTS; }
[0-9]+\.[0-9]+          { COUNT_COLUMNS; yylval.fval = atof(yytext); return FLOAT; }
[0-9]+                  { COUNT_COLUMNS; yylval.ival = atoi(yytext); return INT; }
\"([^\\\"]|\\.)*\"      { COUNT_COLUMNS; yylval.sval = proccess_quoted_string(yytext); return STRING; }// double quote string literal god bless stack overflow
'[a-zA-Z_][a-zA-Z0-9_]* { COUNT_COLUMNS; yylval.sval = strdup(yytext+1); return STRING; }// single quote string
[a-zA-Z_][a-zA-Z0-9_]*  { COUNT_COLUMNS; yylval.sval = strdup(yytext); return NAME; }
!=					    { COUNT_COLUMNS; yylval.sval = strdup(yytext); return BINARY_OPERATOR; }
==					    { COUNT_COLUMNS; yylval.sval = strdup(yytext); return BINARY_OPERATOR; }
[\(\)\{\},\.#=\[\]!-]   { COUNT_COLUMNS; return yytext[0]; }// symbols used directly by parser, - must be separated from other binary operators because it is also a prefix
[+\-\/*^&|<>%]+          { COUNT_COLUMNS; yylval.sval = strdup(yytext); return BINARY_OPERATOR; }
[+\-\/*^&|]+=           { COUNT_COLUMNS; yylval.sval = remove_last_character(yytext); return ASSIGN_BINARY_OPERATOR; }// +=, -= etc
\`.+?\`             	{ COUNT_COLUMNS; yylval.sval = unquote(yytext); return BINARY_OPERATOR; }// function names can be used as operators, for example: [1,2,3] `map` timestwo
%%