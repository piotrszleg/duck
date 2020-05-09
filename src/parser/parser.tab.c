
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "parser.y"

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

// this function doesn't terminate parsing, it needs to be done from parser code
void syntax_error(const char* message){
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
	delete_expression(parsing_result);
	parsing_result=NULL;
}

// called by yacc when parsing fails
void yyerror(const char *message) {
	syntax_error(message);
}

// if item is already at the end of the vector don't add it again
void pointers_vector_push_ignore_duplicate(vector *v, void *item){
    if(vector_top(v)!=item){
        pointers_vector_push(v, item);
    }
}

#define ADD_DEBUG_INFO(expression) expression->line_number=line_number; expression->column_number=column_number;

void name_to_argument(Name* n, Argument* a) {
	a->name=strdup(n->value);
	a->line_number=n->line_number;
	a->column_number=n->column_number;
	a->used_in_closure=false;
}

// true on error
bool process_function_declaration_arguments(FunctionDeclaration* function){
    int arguments_count=vector_count(&function->arguments);
    bool past_optional_arguments=false;
    for(int i=0; i<arguments_count; i++){
        Expression* argument=pointers_vector_get(&function->arguments, i);
        // if one of the arguments is variadic then function is variadic
        switch(argument->type) {
            case e_variadic_argument:
                if(i!=arguments_count-1){
                    syntax_error("Variadic argument must be last in arguments list");
					return true;
                } else {
                	function->variadic=true;
				}
                break;
            case e_optional_argument:
				function->has_optional_arguments=true;
                past_optional_arguments=true;
                break;
            case e_argument:
                if(past_optional_arguments){
                    syntax_error("Optional argument cannot be followed by normal argument");
					return true;
                }
                break;
			default:
				syntax_error("Incorrect argument expression type");
				return true;
        }
    }
	return false;
}

#define YYERROR_VERBOSE 1


/* Line 189 of yacc.c  */
#line 193 "parser.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ENDL = 258,
     IF = 259,
     ELIF = 260,
     ELSE = 261,
     NULL_LITERAL = 262,
     DOT = 263,
     ELLIPSIS = 264,
     ARROW = 265,
     FOUR_DOTS = 266,
     AT = 267,
     QUESTION_MARK = 268,
     INT = 269,
     FLOAT = 270,
     STRING = 271,
     NAME = 272,
     ASSIGN_BINARY_OPERATOR = 273,
     BINARY_OPERATOR = 274,
     PREFIX_OPERATOR = 275
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 125 "parser.y"

	int ival;
	float fval;
	char *sval;
	struct vector* arguments;
	struct Expression* expression;



/* Line 214 of yacc.c  */
#line 259 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 271 "parser.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  67
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   392

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  31
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  39
/* YYNRULES -- Number of rules.  */
#define YYNRULES  88
/* YYNRULES -- Number of states.  */
#define YYNSTATES  145

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   275

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    28,     2,     2,     2,     2,     2,     2,
      26,    27,     2,     2,    21,    30,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    29,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    24,     2,    25,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    22,     2,    23,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     6,     8,    10,    12,    15,    19,
      22,    25,    27,    33,    35,    41,    44,    45,    47,    49,
      51,    53,    55,    57,    59,    61,    63,    65,    67,    69,
      71,    73,    75,    77,    79,    81,    85,    88,    90,    92,
      94,    96,    98,   100,   102,   105,   109,   114,   119,   125,
     130,   133,   136,   142,   149,   152,   159,   165,   169,   171,
     173,   175,   177,   180,   184,   190,   194,   199,   202,   204,
     206,   208,   210,   212,   216,   220,   224,   229,   233,   240,
     246,   250,   254,   257,   260,   264,   266,   267,   270
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      32,     0,    -1,    35,    -1,    -1,    69,    -1,    21,    -1,
      33,    -1,    34,    33,    -1,    35,    34,    39,    -1,    34,
      35,    -1,    35,    34,    -1,    39,    -1,    22,    68,    35,
      68,    23,    -1,    35,    -1,    24,    68,    37,    68,    25,
      -1,    24,    25,    -1,    -1,    40,    -1,    58,    -1,    36,
      -1,    38,    -1,    42,    -1,    61,    -1,    63,    -1,    57,
      -1,    51,    -1,    65,    -1,    66,    -1,    59,    -1,    64,
      -1,    50,    -1,    62,    -1,    49,    -1,    67,    -1,    41,
      -1,    26,    39,    27,    -1,     9,    39,    -1,    60,    -1,
      43,    -1,    44,    -1,    48,    -1,    46,    -1,    45,    -1,
      47,    -1,     8,    60,    -1,    39,     8,    60,    -1,    39,
      13,     8,    60,    -1,    39,    24,    39,    25,    -1,    39,
      13,    24,    39,    25,    -1,     8,    24,    39,    25,    -1,
      39,    28,    -1,    12,    60,    -1,     4,    26,    39,    27,
      39,    -1,     4,    26,    39,    27,    39,    52,    -1,     6,
      39,    -1,     5,    26,    39,    27,    39,    52,    -1,     5,
      26,    39,    27,    39,    -1,    53,    21,    54,    -1,    54,
      -1,    60,    -1,    56,    -1,    55,    -1,    60,     9,    -1,
      60,    29,    39,    -1,    26,    53,    27,    10,    39,    -1,
      60,    10,    39,    -1,    60,     9,    10,    39,    -1,    10,
      39,    -1,    14,    -1,    15,    -1,    16,    -1,     7,    -1,
      17,    -1,    42,    18,    39,    -1,    42,    29,    39,    -1,
      50,    29,    39,    -1,    39,    26,    35,    27,    -1,    39,
      26,    27,    -1,    39,    11,    60,    26,    35,    27,    -1,
      39,    11,    60,    26,    27,    -1,    39,    19,    39,    -1,
      39,    30,    39,    -1,    28,    39,    -1,    30,    39,    -1,
      39,    28,    13,    -1,    69,    -1,    -1,    69,     3,    -1,
       3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   191,   191,   194,   201,   201,   203,   204,   206,   210,
     213,   216,   224,   227,   233,   236,   243,   244,   245,   246,
     247,   248,   249,   250,   251,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,   264,   272,   280,   281,   282,
     283,   284,   285,   286,   289,   297,   306,   315,   324,   333,
     341,   349,   357,   365,   375,   378,   386,   396,   400,   409,
     416,   417,   420,   429,   439,   455,   476,   480,   489,   495,
     501,   509,   516,   524,   538,   549,   559,   567,   576,   585,
     595,   604,   615,   623,   633,   642,   643,   645,   646
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ENDL", "IF", "ELIF", "ELSE",
  "NULL_LITERAL", "DOT", "ELLIPSIS", "ARROW", "FOUR_DOTS", "AT",
  "QUESTION_MARK", "INT", "FLOAT", "STRING", "NAME",
  "ASSIGN_BINARY_OPERATOR", "BINARY_OPERATOR", "PREFIX_OPERATOR", "','",
  "'{'", "'}'", "'['", "']'", "'('", "')'", "'!'", "'='", "'-'", "$accept",
  "program", "lines_separator", "lines_separators", "lines", "block",
  "table_contents", "table", "expression", "parentheses", "unpack",
  "identifier", "self_member_access", "member_access",
  "null_conditional_member_access", "indexer", "null_conditional_indexer",
  "self_indexer", "return", "macro", "conditional", "conditional_else",
  "arguments", "argument", "variadic_argument", "optional_argument",
  "function", "literal", "null_literal", "name", "assignment",
  "macro_declaration", "call", "message", "binary", "prefix",
  "return_if_error", "OPT_ENDLS", "ENDLS", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,    44,   123,   125,    91,    93,    40,    41,    33,    61,
      45
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    31,    32,    32,    33,    33,    34,    34,    35,    35,
      35,    35,    36,    37,    38,    38,    39,    39,    39,    39,
      39,    39,    39,    39,    39,    39,    39,    39,    39,    39,
      39,    39,    39,    39,    39,    40,    41,    42,    42,    42,
      42,    42,    42,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    51,    52,    52,    52,    53,    53,    54,
      54,    54,    55,    56,    57,    57,    57,    57,    58,    58,
      58,    59,    60,    61,    61,    62,    63,    63,    64,    64,
      65,    65,    66,    66,    67,    68,    68,    69,    69
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     1,     1,     1,     2,     3,     2,
       2,     1,     5,     1,     5,     2,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     2,     1,     1,     1,
       1,     1,     1,     1,     2,     3,     4,     4,     5,     4,
       2,     2,     5,     6,     2,     6,     5,     3,     1,     1,
       1,     1,     2,     3,     5,     3,     4,     2,     1,     1,
       1,     1,     1,     3,     3,     3,     4,     3,     6,     5,
       3,     3,     2,     2,     3,     1,     0,     2,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      16,    88,     0,    71,     0,    16,    16,     0,    68,    69,
      70,    72,     5,    86,    86,    16,    16,    16,     0,     6,
      16,     2,    19,    20,    11,    17,    34,    21,    38,    39,
      42,    41,    43,    40,    32,    30,    25,    24,    18,    28,
      37,    22,    31,    23,    29,    26,    27,    33,     4,    16,
      16,    44,    36,    67,    51,    16,    85,    15,    16,     0,
       0,    58,    61,    60,    37,    82,    83,     1,     6,     9,
      10,     0,     0,     0,    16,    16,    16,    50,    16,    16,
      16,    16,     0,    16,    87,     0,     0,    86,    13,    86,
      35,     0,     0,    62,    16,     7,     8,    45,     0,     0,
      16,    80,     0,    77,     0,    84,    81,    73,    74,    75,
      16,    65,    16,    49,     0,     4,     0,    57,    59,    16,
      63,    16,    46,     0,    47,    76,    66,    52,    12,    14,
      62,    64,    79,     0,    48,     0,    16,    53,    78,    16,
      54,     0,    16,    56,    55
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    18,    19,    20,    21,    22,    89,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,   137,    60,    61,    62,    63,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    55,    48
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -60
static const yytype_int16 yypact[] =
{
     132,   -60,    -8,   -60,    40,   269,   269,     2,   -60,   -60,
     -60,   -60,   -60,    44,    11,   269,   269,   269,    49,   -60,
     244,    17,   -60,   -60,   362,   -60,   -60,    25,   -60,   -60,
     -60,   -60,   -60,   -60,   -60,    32,   -60,   -60,   -60,   -60,
      50,   -60,   -60,   -60,   -60,   -60,   -60,   -60,    48,   269,
     269,   -60,   362,   362,   -60,   244,    48,   -60,   244,    85,
      -6,   -60,   -60,   -60,    13,   362,   362,   -60,   -60,    17,
     160,     2,     2,    31,   269,   269,   188,    53,   269,   269,
     269,   269,    52,   269,   -60,   279,   300,    17,    17,    44,
     -60,     2,    57,    52,   269,   -60,   362,   -60,    37,     2,
     269,   362,   310,   -60,     4,   -60,   362,   362,   362,   362,
     269,   362,   269,   -60,    45,    48,    58,   -60,     0,   269,
     362,   216,   -60,   331,   -60,   -60,   362,    22,   -60,   -60,
     -60,   362,   -60,     5,   -60,    56,   269,   -60,   -60,   269,
     362,   341,   269,    22,   -60
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -60,   -60,   -14,   -16,   -18,   -60,   -60,   -60,    -5,   -60,
     -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,
     -60,   -59,   -60,    -4,   -60,   -60,   -60,   -60,   -60,     9,
     -60,   -60,   -60,   -60,   -60,   -60,   -60,     3,   -10
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -60
static const yytype_int16 yytable[] =
{
      52,    53,    69,    56,    56,    70,    68,     1,     1,   130,
      59,    65,    66,    51,     1,    91,    54,    58,    49,    11,
       1,    92,    93,    83,    64,    12,    12,   135,   136,    94,
      71,   125,   138,    72,   -59,    73,    57,    87,    12,    99,
      88,    74,    94,    79,    85,    86,    75,     1,    76,    67,
      77,    84,    78,    70,    80,   100,    95,    11,   104,    82,
      83,    81,   110,   121,    50,    96,   105,   119,   128,   101,
     102,    70,    70,   106,   107,   108,   109,   115,   111,    56,
      97,    98,   139,   129,   144,     0,     0,   117,    70,   120,
     114,     0,   116,    71,     0,   123,    72,     0,    73,     0,
     118,     0,     0,   133,    74,   126,     0,   127,   122,    75,
       0,    76,    90,    77,   131,    78,     0,    70,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   140,    -3,     0,   141,     1,     2,   143,     0,     3,
       4,     5,     6,     0,     7,     0,     8,     9,    10,    11,
       0,     0,     0,    12,    13,     0,    14,     0,    15,     0,
      16,     0,    17,     1,     2,     0,     0,     3,     4,     5,
       6,   -16,     7,   -16,     8,     9,    10,    11,     0,   -16,
       0,    12,    13,     0,    14,     0,    15,     0,    16,     0,
      17,     1,     2,     0,     0,     3,     4,     5,     6,     0,
       7,     0,     8,     9,    10,    11,     0,     0,     0,    12,
      13,     0,    14,     0,    15,   103,    16,     0,    17,     1,
       2,     0,     0,     3,     4,     5,     6,     0,     7,     0,
       8,     9,    10,    11,     0,     0,     0,    12,    13,     0,
      14,     0,    15,   132,    16,     0,    17,     1,     2,     0,
       0,     3,     4,     5,     6,     0,     7,     0,     8,     9,
      10,    11,     0,     0,     0,    12,    13,     0,    14,     0,
      15,     0,    16,     2,    17,     0,     3,     4,     5,     6,
       0,     7,     0,     8,     9,    10,    11,    71,     0,     0,
      72,    13,    73,    14,     0,    15,     0,    16,    74,    17,
       0,     0,     0,    75,     0,    76,   112,    77,    71,    78,
       0,    72,     0,    73,     0,     0,     0,     0,    71,    74,
       0,    72,     0,    73,    75,   113,    76,     0,    77,    74,
      78,     0,     0,     0,    75,   124,    76,     0,    77,    71,
      78,     0,    72,     0,    73,     0,     0,     0,     0,    71,
      74,     0,    72,     0,    73,    75,   134,    76,     0,    77,
      74,    78,     0,     0,     0,    75,     0,    76,   142,    77,
      71,    78,     0,    72,     0,    73,     0,     0,     0,     0,
       0,    74,     0,     0,     0,     0,    75,     0,    76,     0,
      77,     0,    78
};

static const yytype_int16 yycheck[] =
{
       5,     6,    20,    13,    14,    21,    20,     3,     3,     9,
      15,    16,    17,     4,     3,    21,     7,    14,    26,    17,
       3,    27,     9,    10,    15,    21,    21,     5,     6,    29,
       8,    27,    27,    11,    21,    13,    25,    55,    21,     8,
      58,    19,    29,    18,    49,    50,    24,     3,    26,     0,
      28,     3,    30,    69,    29,    24,    70,    17,    76,     9,
      10,    29,    10,    26,    24,    70,    13,    10,    23,    74,
      75,    87,    88,    78,    79,    80,    81,    87,    83,    89,
      71,    72,    26,    25,   143,    -1,    -1,    91,   104,    94,
      87,    -1,    89,     8,    -1,   100,    11,    -1,    13,    -1,
      91,    -1,    -1,   121,    19,   110,    -1,   112,    99,    24,
      -1,    26,    27,    28,   119,    30,    -1,   133,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   136,     0,    -1,   139,     3,     4,   142,    -1,     7,
       8,     9,    10,    -1,    12,    -1,    14,    15,    16,    17,
      -1,    -1,    -1,    21,    22,    -1,    24,    -1,    26,    -1,
      28,    -1,    30,     3,     4,    -1,    -1,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    -1,    19,
      -1,    21,    22,    -1,    24,    -1,    26,    -1,    28,    -1,
      30,     3,     4,    -1,    -1,     7,     8,     9,    10,    -1,
      12,    -1,    14,    15,    16,    17,    -1,    -1,    -1,    21,
      22,    -1,    24,    -1,    26,    27,    28,    -1,    30,     3,
       4,    -1,    -1,     7,     8,     9,    10,    -1,    12,    -1,
      14,    15,    16,    17,    -1,    -1,    -1,    21,    22,    -1,
      24,    -1,    26,    27,    28,    -1,    30,     3,     4,    -1,
      -1,     7,     8,     9,    10,    -1,    12,    -1,    14,    15,
      16,    17,    -1,    -1,    -1,    21,    22,    -1,    24,    -1,
      26,    -1,    28,     4,    30,    -1,     7,     8,     9,    10,
      -1,    12,    -1,    14,    15,    16,    17,     8,    -1,    -1,
      11,    22,    13,    24,    -1,    26,    -1,    28,    19,    30,
      -1,    -1,    -1,    24,    -1,    26,    27,    28,     8,    30,
      -1,    11,    -1,    13,    -1,    -1,    -1,    -1,     8,    19,
      -1,    11,    -1,    13,    24,    25,    26,    -1,    28,    19,
      30,    -1,    -1,    -1,    24,    25,    26,    -1,    28,     8,
      30,    -1,    11,    -1,    13,    -1,    -1,    -1,    -1,     8,
      19,    -1,    11,    -1,    13,    24,    25,    26,    -1,    28,
      19,    30,    -1,    -1,    -1,    24,    -1,    26,    27,    28,
       8,    30,    -1,    11,    -1,    13,    -1,    -1,    -1,    -1,
      -1,    19,    -1,    -1,    -1,    -1,    24,    -1,    26,    -1,
      28,    -1,    30
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     7,     8,     9,    10,    12,    14,    15,
      16,    17,    21,    22,    24,    26,    28,    30,    32,    33,
      34,    35,    36,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    69,    26,
      24,    60,    39,    39,    60,    68,    69,    25,    68,    39,
      53,    54,    55,    56,    60,    39,    39,     0,    33,    35,
      34,     8,    11,    13,    19,    24,    26,    28,    30,    18,
      29,    29,     9,    10,     3,    39,    39,    35,    35,    37,
      27,    21,    27,     9,    29,    33,    39,    60,    60,     8,
      24,    39,    39,    27,    35,    13,    39,    39,    39,    39,
      10,    39,    27,    25,    68,    69,    68,    54,    60,    10,
      39,    26,    60,    39,    25,    27,    39,    39,    23,    25,
       9,    39,    27,    35,    25,     5,     6,    52,    27,    26,
      39,    39,    27,    39,    52
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 191 "parser.y"
    { 
		parsing_result = (yyvsp[(1) - (1)].expression);
	;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 194 "parser.y"
    {
		Empty* e=new_empty();
		ADD_DEBUG_INFO(e)
		parsing_result=(Expression*)e;
	;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 206 "parser.y"
    {
		pointers_vector_push_ignore_duplicate(&((Block*)(yyvsp[(1) - (3)].expression))->lines, (yyvsp[(3) - (3)].expression));
		(yyval.expression)=(yyvsp[(1) - (3)].expression);
	;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 210 "parser.y"
    {
		(yyval.expression)=(yyvsp[(2) - (2)].expression);
	;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 213 "parser.y"
    {
		(yyval.expression)=(yyvsp[(1) - (2)].expression);
	;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 216 "parser.y"
    {
		Block* b=new_block();
		ADD_DEBUG_INFO(b)
		pointers_vector_push_ignore_duplicate(&b->lines, (yyvsp[(1) - (1)].expression));
		(yyval.expression)=(Expression*)b;
	;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 224 "parser.y"
    { (yyval.expression)=(yyvsp[(3) - (5)].expression); ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 227 "parser.y"
    {
		(yyval.expression)=(yyvsp[(1) - (1)].expression); 
		(yyval.expression)->type=e_table_literal;
	;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 233 "parser.y"
    { 
		(yyval.expression)=(yyvsp[(3) - (5)].expression);
	;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 236 "parser.y"
    {
		Block* b=new_block();
		ADD_DEBUG_INFO(b)
		b->type=e_table_literal;
		(yyval.expression)=(Expression*)b;
	;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 264 "parser.y"
    {
		Parentheses* p=new_parentheses();
		ADD_DEBUG_INFO(p)
		p->value=(yyvsp[(2) - (3)].expression);
		(yyval.expression)=(Expression*)p;
	;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 272 "parser.y"
    {
		Unpack* u=new_unpack();
		ADD_DEBUG_INFO(u)
		u->value=(Expression*)(yyvsp[(2) - (2)].expression);
		(yyval.expression)=(Expression*)u;
	;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 289 "parser.y"
    {
		SelfMemberAccess* ma=new_self_member_access();
		ADD_DEBUG_INFO(ma)
		ma->right=(Name*)(yyvsp[(2) - (2)].expression);
		(yyval.expression)=(Expression*)ma;
	;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 297 "parser.y"
    {
		MemberAccess* ma=new_member_access();
		ADD_DEBUG_INFO(ma)
		ma->left=(yyvsp[(1) - (3)].expression);
		ma->right=(Name*)(yyvsp[(3) - (3)].expression);
		(yyval.expression)=(Expression*)ma;
	;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 306 "parser.y"
    {
		NullConditionalMemberAccess* ma=new_null_conditional_member_access();
		ADD_DEBUG_INFO(ma)
		ma->left=(yyvsp[(1) - (4)].expression);
		ma->right=(Name*)(yyvsp[(4) - (4)].expression);
		(yyval.expression)=(Expression*)ma;
	;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 315 "parser.y"
    {
		Indexer* i=new_indexer();
		ADD_DEBUG_INFO(i)
		i->left=(yyvsp[(1) - (4)].expression);
		i->right=(yyvsp[(3) - (4)].expression);
		(yyval.expression)=(Expression*)i;
	;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 324 "parser.y"
    {
		NullConditionalIndexer* i=new_null_conditional_indexer();
		ADD_DEBUG_INFO(i)
		i->left=(yyvsp[(1) - (5)].expression);
		i->right=(yyvsp[(4) - (5)].expression);
		(yyval.expression)=(Expression*)i;
	;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 333 "parser.y"
    {
		SelfIndexer* i=new_self_indexer();
		ADD_DEBUG_INFO(i)
		i->right=(yyvsp[(3) - (4)].expression);
		(yyval.expression)=(Expression*)i;
	;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 341 "parser.y"
    {
		FunctionReturn* r=new_function_return();
		ADD_DEBUG_INFO(r)
		r->value=(yyvsp[(1) - (2)].expression);
		(yyval.expression)=(Expression*)r;
	;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 349 "parser.y"
    {
		Macro* m=new_macro();
		ADD_DEBUG_INFO(m)
		m->identifier=(Name*)(yyvsp[(2) - (2)].expression);
		(yyval.expression)=(Expression*)m;
	;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 357 "parser.y"
    {	
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[(3) - (5)].expression);
		c->ontrue=(yyvsp[(5) - (5)].expression);
		c->onfalse=(Expression*)new_empty();
		(yyval.expression)=(Expression*)c;
	;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 365 "parser.y"
    {
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[(3) - (6)].expression);
		c->ontrue=(yyvsp[(5) - (6)].expression);
		c->onfalse=(yyvsp[(6) - (6)].expression);
		(yyval.expression)=(Expression*)c;
	;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 375 "parser.y"
    {
		(yyval.expression)=(yyvsp[(2) - (2)].expression);
	;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 378 "parser.y"
    {
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[(3) - (6)].expression);
		c->ontrue=(yyvsp[(5) - (6)].expression);
		c->onfalse=(yyvsp[(6) - (6)].expression);
		(yyval.expression)=(Expression*)c;
	;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 386 "parser.y"
    {
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[(3) - (5)].expression);
		c->ontrue=(yyvsp[(5) - (5)].expression);
		c->onfalse=(Expression*)new_empty();
		(yyval.expression)=(Expression*)c;
	;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 396 "parser.y"
    {
		pointers_vector_push((yyvsp[(1) - (3)].arguments), (yyvsp[(3) - (3)].expression));
		(yyval.arguments)=(yyvsp[(1) - (3)].arguments);
	;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 400 "parser.y"
    {
		vector* arguments=malloc(sizeof(vector));
		CHECK_ALLOCATION(arguments);
		vector_init(arguments, sizeof(Argument), 8);
		pointers_vector_push(arguments, (yyvsp[(1) - (1)].expression));
		(yyval.arguments)=arguments;
	;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 409 "parser.y"
    {
		Argument* a=new_argument();
		Name* name=(Name*)(yyvsp[(1) - (1)].expression);
		name_to_argument(name, a);
		delete_expression((Expression*)name);
		(yyval.expression)=(Expression*)a;
	;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 416 "parser.y"
    { (yyval.expression)=(yyvsp[(1) - (1)].expression); ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 417 "parser.y"
    { (yyval.expression)=(yyvsp[(1) - (1)].expression); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 420 "parser.y"
    {
		VariadicArgument* a=new_variadic_argument();
		Name* name=(Name*)(yyvsp[(1) - (2)].expression);
		name_to_argument(name, (Argument*)a);
		delete_expression((Expression*)name);
		(yyval.expression)=(Expression*)a;
	;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 429 "parser.y"
    {
		OptionalArgument* a=new_optional_argument();
		Name* name=(Name*)(yyvsp[(1) - (3)].expression);
		name_to_argument(name, (Argument*)a);
		delete_expression((Expression*)name);
		a->value=(yyvsp[(3) - (3)].expression);
		(yyval.expression)=(Expression*)a;
	;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 439 "parser.y"
    {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		f->arguments=*(vector*)(yyvsp[(2) - (5)].arguments);
		free((yyvsp[(2) - (5)].arguments));

		f->variadic=false;
		f->body=(yyvsp[(5) - (5)].expression);
		if(process_function_declaration_arguments(f)){
			// function's arguments are formed incorrectly
			delete_expression((Expression*)f);
			YYABORT;
		}
		// one argument function's don't need to be processed using this function
		(yyval.expression)=(Expression*)f;
	;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 455 "parser.y"
    {
		// changing name above to argument causes assignments to be incorrectly parsed
		#define ONE_ARGUMENT_FUNCTION(result, argument, _body, _variadic) \
			FunctionDeclaration* f=new_function_declaration(); \
			ADD_DEBUG_INFO(f) \
			\
			/* transform name into argument and delete it */ \
			Argument* a=new_argument(); \
			Name* name=(Name*)argument; \
			name_to_argument(name, a); \
			delete_expression((Expression*)name); \
			if (_variadic) { \
				a->type=e_variadic_argument; \
			} \
			\
			pointers_vector_push(&f->arguments, a); \
			f->variadic=_variadic; \
			f->body=_body; \
			result=(Expression*)f;
		ONE_ARGUMENT_FUNCTION((yyval.expression), (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), false)
	;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 476 "parser.y"
    {
		ONE_ARGUMENT_FUNCTION((yyval.expression), (yyvsp[(1) - (4)].expression), (yyvsp[(4) - (4)].expression), true)
		#undef ONE_ARGUMENT_FUNCTION
	;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 480 "parser.y"
    {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		f->variadic=false;
		f->body=(yyvsp[(2) - (2)].expression);
		(yyval.expression)=(Expression*)f;
	;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 489 "parser.y"
    {
		IntLiteral* l=new_int_literal();
		ADD_DEBUG_INFO(l)
		l->value=(yyvsp[(1) - (1)].ival);
		(yyval.expression)=(Expression*)l;
	;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 495 "parser.y"
    { 
		FloatLiteral* l=new_float_literal();
		ADD_DEBUG_INFO(l)
		l->value=(yyvsp[(1) - (1)].fval);
		(yyval.expression)=(Expression*)l;
	;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 501 "parser.y"
    { 
		StringLiteral* l=new_string_literal();
		ADD_DEBUG_INFO(l)
		l->value=(yyvsp[(1) - (1)].sval);
		(yyval.expression)=(Expression*)l;
	;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 509 "parser.y"
    {
		NullLiteral* n=new_null_literal();
		ADD_DEBUG_INFO(n)
		(yyval.expression)=(Expression*)n;
	;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 516 "parser.y"
    {
		Name* n=new_name();
		ADD_DEBUG_INFO(n)
		n->value=(yyvsp[(1) - (1)].sval);
		(yyval.expression)=(Expression*)n;
	  ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 525 "parser.y"
    {
		Assignment* a=new_assignment();
		ADD_DEBUG_INFO(a)
		a->left=(Expression*)(yyvsp[(1) - (3)].expression);
		Binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=copy_expression((yyvsp[(1) - (3)].expression));
		u->op=(yyvsp[(2) - (3)].sval);
		u->right=(yyvsp[(3) - (3)].expression);
		a->right=(Expression*)u;
		a->used_in_closure=false;
		(yyval.expression)=(Expression*)a;
	;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 539 "parser.y"
    {
		Assignment* a=new_assignment();
		ADD_DEBUG_INFO(a)
		a->left=(Expression*)(yyvsp[(1) - (3)].expression);
		a->right=(yyvsp[(3) - (3)].expression);
		a->used_in_closure=false;
		(yyval.expression)=(Expression*)a;
	;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 550 "parser.y"
    {
		MacroDeclaration* md=new_macro_declaration();
		ADD_DEBUG_INFO(md)
		md->left=(Macro*)(yyvsp[(1) - (3)].expression);
		md->right=(yyvsp[(3) - (3)].expression);
		(yyval.expression)=(Expression*)md;
	;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 559 "parser.y"
    {
		FunctionCall* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->called=(yyvsp[(1) - (4)].expression);
		c->arguments=(Block*)(yyvsp[(3) - (4)].expression);
		c->arguments->type=e_table_literal;
		(yyval.expression)=(Expression*)c;
	;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 567 "parser.y"
    {
		FunctionCall* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->called=(yyvsp[(1) - (3)].expression);
		c->arguments=(Block*)new_block();
		(yyval.expression)=(Expression*)c;
	;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 576 "parser.y"
    {
		Message* m=new_message();
		ADD_DEBUG_INFO(m)
		m->messaged_object=(yyvsp[(1) - (6)].expression);
		m->message_name=(Name*)(yyvsp[(3) - (6)].expression);
		m->arguments=(Block*)(yyvsp[(5) - (6)].expression);
		m->arguments->type=e_table_literal;
		(yyval.expression)=(Expression*)m;
	;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 585 "parser.y"
    {
		Message* m=new_message();
		ADD_DEBUG_INFO(m)
		m->messaged_object=(yyvsp[(1) - (5)].expression);
		m->message_name=(Name*)(yyvsp[(3) - (5)].expression);
		m->arguments=(Block*)new_block();
		(yyval.expression)=(Expression*)m;
	;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 596 "parser.y"
    {
		Binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=(yyvsp[(1) - (3)].expression);
		u->op=(yyvsp[(2) - (3)].sval);
		u->right=(yyvsp[(3) - (3)].expression);
		(yyval.expression)=(Expression*)u;
	;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 605 "parser.y"
    {
		Binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=(yyvsp[(1) - (3)].expression);
		u->op=strdup("-");
		u->right=(yyvsp[(3) - (3)].expression);
		(yyval.expression)=(Expression*)u;
	;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 616 "parser.y"
    {
		Prefix* p=new_prefix();
		ADD_DEBUG_INFO(p)
		p->op=strdup("!");
		p->right=(yyvsp[(2) - (2)].expression);
		(yyval.expression)=(Expression*)p;
	;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 624 "parser.y"
    {
		Prefix* p=new_prefix();
		ADD_DEBUG_INFO(p)
		p->op=strdup("-");
		p->right=(yyvsp[(2) - (2)].expression);
		(yyval.expression)=(Expression*)p;
	;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 634 "parser.y"
    {
		ReturnIfError* r=new_return_if_error();
		ADD_DEBUG_INFO(r)
		r->value=(yyvsp[(1) - (3)].expression);
		(yyval.expression)=(Expression*)r;
	;}
    break;



/* Line 1455 of yacc.c  */
#line 2313 "parser.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 647 "parser.y"


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
