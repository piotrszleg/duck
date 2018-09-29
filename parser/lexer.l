snazzle.l
%{
#include "parser.tab.h"
int line_num = 1;
%}
%option noyywrap
%s COMMENT
%%
[ \t]          ;
\n             { ++line_num; return ENDL; }
#.*            ;
if             { return IF; }
elif             { return ELIF; }
else             { return ELSE; }
[0-9]+\.[0-9]+ { yylval.fval = atof(yytext); return FLOAT; }
[0-9]+         { yylval.ival = atoi(yytext); return INT; }
\"[a-zA-Z0-9]+\"   { yylval.sval = strdup(yytext); return STRING; }
[a-zA-Z0-9]+   { yylval.sval = strdup(yytext); return NAME; }
[+\-\/*^&|]         { yylval.sval = strdup(yytext); return UNARY_OPERATOR; }
[\(\)\{\}=,#]   { return yytext[0]; }
->             { return ARROW; }
.              ;
%%