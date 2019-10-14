
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
void pointers_vector_push_ignore_duplicate(vector *v, void *item){
    if(vector_top(v)!=item){
        pointers_vector_push(v, item);
    }
}

#define ADD_DEBUG_INFO(exp) exp->line_number=line_number; exp->column_number=column_number;

argument* name_to_argument(name* n) {
	argument* a=new_argument();
	ADD_DEBUG_INFO(a)
	a->name=strdup(n->value);
	a->used_in_closure=false;
	delete_expression((expression*)n);
	return a;
}

#define YYERROR_VERBOSE 1


/* Line 189 of yacc.c  */
#line 112 "parser.tab.c"

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
#line 44 "parser.y"

	int ival;
	float fval;
	char *sval;
	struct vector* args;
	struct expression* exp;



/* Line 214 of yacc.c  */
#line 178 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 190 "parser.tab.c"

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
#define YYFINAL  62
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   343

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  31
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  36
/* YYNRULES -- Number of rules.  */
#define YYNRULES  83
/* YYNRULES -- Number of states.  */
#define YYNSTATES  140

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
      71,    73,    75,    77,    79,    83,    85,    87,    89,    91,
      93,    95,    97,   100,   104,   109,   114,   120,   125,   128,
     131,   137,   144,   147,   154,   160,   164,   166,   168,   174,
     181,   186,   190,   193,   195,   197,   199,   201,   203,   207,
     211,   215,   220,   224,   231,   237,   241,   245,   248,   251,
     255,   257,   258,   261
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      32,     0,    -1,    35,    -1,    -1,    66,    -1,    21,    -1,
      33,    -1,    34,    33,    -1,    35,    34,    39,    -1,    34,
      35,    -1,    35,    34,    -1,    39,    -1,    22,    65,    35,
      65,    23,    -1,    35,    -1,    24,    65,    37,    65,    25,
      -1,    24,    25,    -1,    -1,    40,    -1,    55,    -1,    36,
      -1,    38,    -1,    41,    -1,    58,    -1,    60,    -1,    54,
      -1,    50,    -1,    62,    -1,    63,    -1,    56,    -1,    61,
      -1,    49,    -1,    59,    -1,    48,    -1,    64,    -1,    26,
      39,    27,    -1,    57,    -1,    42,    -1,    43,    -1,    47,
      -1,    45,    -1,    44,    -1,    46,    -1,     8,    57,    -1,
      41,     8,    57,    -1,    41,    13,     8,    57,    -1,    41,
      24,    39,    25,    -1,    41,    13,    24,    39,    25,    -1,
       8,    24,    39,    25,    -1,    39,    28,    -1,    12,    57,
      -1,     4,    26,    39,    27,    39,    -1,     4,    26,    39,
      27,    39,    51,    -1,     6,    39,    -1,     5,    26,    39,
      27,    39,    51,    -1,     5,    26,    39,    27,    39,    -1,
      52,    21,    53,    -1,    53,    -1,    17,    -1,    26,    52,
      27,    10,    39,    -1,    26,    52,     9,    27,    10,    39,
      -1,    57,     9,    10,    39,    -1,    57,    10,    39,    -1,
      10,    39,    -1,    14,    -1,    15,    -1,    16,    -1,     7,
      -1,    17,    -1,    41,    18,    39,    -1,    41,    29,    39,
      -1,    49,    29,    39,    -1,    39,    26,    35,    27,    -1,
      39,    26,    27,    -1,    39,    11,    57,    26,    35,    27,
      -1,    39,    11,    57,    26,    27,    -1,    39,    19,    39,
      -1,    39,    30,    39,    -1,    28,    39,    -1,    30,    39,
      -1,    39,    28,    13,    -1,    66,    -1,    -1,    66,     3,
      -1,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   107,   107,   110,   117,   117,   119,   120,   122,   126,
     129,   132,   140,   143,   149,   152,   159,   160,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   179,   187,   188,   189,   190,   191,
     192,   193,   196,   204,   213,   222,   231,   240,   248,   256,
     264,   272,   282,   285,   293,   303,   307,   316,   325,   335,
     345,   353,   361,   370,   376,   382,   390,   397,   405,   419,
     430,   440,   448,   457,   466,   476,   485,   496,   504,   514,
     523,   524,   526,   527
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
  "table_contents", "table", "expression", "parentheses", "identifier",
  "self_member_access", "member_access", "null_conditional_member_access",
  "indexer", "null_conditional_indexer", "self_indexer", "return", "macro",
  "conditional", "conditional_else", "arguments", "argument", "function",
  "literal", "null", "name", "assignment", "macro_declaration", "call",
  "message", "binary", "prefix", "return_if_error", "OPT_ENDLS", "ENDLS", 0
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
      39,    39,    39,    39,    40,    41,    41,    41,    41,    41,
      41,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    50,    51,    51,    51,    52,    52,    53,    54,    54,
      54,    54,    54,    55,    55,    55,    56,    57,    58,    58,
      59,    60,    60,    61,    61,    62,    62,    63,    63,    64,
      65,    65,    66,    66
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     1,     1,     1,     2,     3,     2,
       2,     1,     5,     1,     5,     2,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     1,     1,     1,     1,     1,
       1,     1,     2,     3,     4,     4,     5,     4,     2,     2,
       5,     6,     2,     6,     5,     3,     1,     1,     5,     6,
       4,     3,     2,     1,     1,     1,     1,     1,     3,     3,
       3,     4,     3,     6,     5,     3,     3,     2,     2,     3,
       1,     0,     2,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      16,    83,     0,    66,     0,    16,     0,    63,    64,    65,
      67,     5,    81,    81,    16,    16,    16,     0,     6,    16,
       2,    19,    20,    11,    17,    21,    36,    37,    40,    39,
      41,    38,    32,    30,    25,    24,    18,    28,    35,    22,
      31,    23,    29,    26,    27,    33,     4,    16,    16,    42,
      62,    49,    16,    80,    15,    16,    67,     0,     0,    56,
      77,    78,     1,     6,     9,    10,     0,    16,    16,    48,
      16,     0,     0,    16,    16,    16,    16,     0,    16,    82,
       0,     0,    81,    13,    81,    34,     0,     0,     0,     7,
       8,     0,    75,    72,     0,    79,    76,    43,     0,    16,
      68,     0,    69,    70,    16,    61,    16,    47,     0,     4,
       0,     0,    57,    55,    16,    16,    71,    44,     0,    45,
      60,    50,    12,    14,    16,    58,    74,     0,    46,     0,
      16,    51,    59,    73,    16,    52,     0,    16,    54,    53
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    17,    18,    19,    20,    21,    84,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
     131,    58,    59,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    52,    46
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -53
static const yytype_int16 yypact[] =
{
     123,   -53,   -22,   -53,    30,   260,     4,   -53,   -53,   -53,
     -53,   -53,    16,    15,   285,   260,   260,    24,   -53,   235,
      45,   -53,   -53,    74,   -53,     7,   -53,   -53,   -53,   -53,
     -53,   -53,   -53,    -2,   -53,   -53,   -53,   -53,    46,   -53,
     -53,   -53,   -53,   -53,   -53,   -53,    47,   260,   260,   -53,
      74,   -53,   235,    47,   -53,   235,    25,    61,   107,   -53,
      74,    74,   -53,   -53,    45,   151,     4,   260,   179,    40,
     260,     4,    20,   260,   260,   260,   260,    23,   260,   -53,
      84,   293,    45,    45,    16,   -53,    31,    42,    51,   -53,
      74,    41,    74,   -53,     2,   -53,    74,   -53,     4,   260,
      74,   303,    74,    74,   260,    74,   260,   -53,    34,    47,
      52,    71,   -53,   -53,   260,   207,   -53,   -53,   313,   -53,
      74,    11,   -53,   -53,   260,    74,   -53,     5,   -53,    57,
     260,   -53,    74,   -53,   260,    74,    94,   260,    11,   -53
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -53,   -53,   -16,   -19,   -17,   -53,   -53,   -53,    -5,   -53,
     -53,   -53,   -53,   -53,   -53,   -53,   -53,   -53,   -53,   -53,
     -52,   -53,     3,   -53,   -53,   -53,     8,   -53,   -53,   -53,
     -53,   -53,   -53,   -53,     0,    -6
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -58
static const yytype_int16 yytable[] =
{
      50,    65,    64,    63,    47,     1,    53,    53,     1,    57,
      60,    61,    49,    55,    51,    71,   129,   130,     1,     1,
      72,    10,    66,    11,    62,    73,    11,    76,    98,   116,
      67,    74,   133,   104,   -57,    82,    75,    68,    83,    69,
      54,    70,    80,    81,    99,    65,   -57,    10,     1,    89,
      79,    94,   -57,    95,    48,    77,    78,   122,   111,   112,
      90,   114,    92,    65,    65,    96,    11,   115,   100,   101,
     102,   103,    66,   105,    91,    65,   109,   123,    53,    97,
      67,   124,   108,   134,   110,    66,   139,    68,    85,    69,
     113,    70,     0,    67,   118,    66,     0,     0,   127,   120,
      68,   121,    69,    67,    70,    66,   117,     0,    65,   125,
      68,   106,    69,    67,    70,     0,    86,     0,     0,   132,
      68,   137,    69,    -3,    70,   135,     1,     2,    87,   136,
       3,     4,   138,     5,    88,     6,     0,     7,     8,     9,
      10,     0,     0,     0,    11,    12,     0,    13,     0,    14,
       0,    15,     0,    16,     1,     2,     0,     0,     3,     4,
       0,     5,   -16,     6,     0,     7,     8,     9,    10,     0,
     -16,     0,    11,    12,     0,    13,     0,    14,     0,    15,
       0,    16,     1,     2,     0,     0,     3,     4,     0,     5,
       0,     6,     0,     7,     8,     9,    10,     0,     0,     0,
      11,    12,     0,    13,     0,    14,    93,    15,     0,    16,
       1,     2,     0,     0,     3,     4,     0,     5,     0,     6,
       0,     7,     8,     9,    10,     0,     0,     0,    11,    12,
       0,    13,     0,    14,   126,    15,     0,    16,     1,     2,
       0,     0,     3,     4,     0,     5,     0,     6,     0,     7,
       8,     9,    10,     0,     0,     0,    11,    12,     0,    13,
       0,    14,     0,    15,     2,    16,     0,     3,     4,     0,
       5,     0,     6,     0,     7,     8,     9,    10,     0,     0,
       0,     0,    12,     0,    13,     0,    14,     0,    15,     2,
      16,     0,     3,     4,     0,     5,     0,     6,     0,     7,
       8,     9,    56,     0,    66,     0,     0,    12,     0,    13,
       0,    14,    67,    15,    66,    16,     0,     0,   107,    68,
       0,    69,    67,    70,    66,     0,     0,     0,   119,    68,
       0,    69,    67,    70,     0,     0,     0,     0,   128,    68,
       0,    69,     0,    70
};

static const yytype_int16 yycheck[] =
{
       5,    20,    19,    19,    26,     3,    12,    13,     3,    14,
      15,    16,     4,    13,     6,     8,     5,     6,     3,     3,
      13,    17,    11,    21,     0,    18,    21,    29,     8,    27,
      19,    24,    27,    10,     9,    52,    29,    26,    55,    28,
      25,    30,    47,    48,    24,    64,    21,    17,     3,    65,
       3,    68,    27,    13,    24,     9,    10,    23,    27,    17,
      65,    10,    67,    82,    83,    70,    21,    26,    73,    74,
      75,    76,    11,    78,    66,    94,    82,    25,    84,    71,
      19,    10,    82,    26,    84,    11,   138,    26,    27,    28,
      87,    30,    -1,    19,    99,    11,    -1,    -1,   115,   104,
      26,   106,    28,    19,    30,    11,    98,    -1,   127,   114,
      26,    27,    28,    19,    30,    -1,     9,    -1,    -1,   124,
      26,    27,    28,     0,    30,   130,     3,     4,    21,   134,
       7,     8,   137,    10,    27,    12,    -1,    14,    15,    16,
      17,    -1,    -1,    -1,    21,    22,    -1,    24,    -1,    26,
      -1,    28,    -1,    30,     3,     4,    -1,    -1,     7,     8,
      -1,    10,    11,    12,    -1,    14,    15,    16,    17,    -1,
      19,    -1,    21,    22,    -1,    24,    -1,    26,    -1,    28,
      -1,    30,     3,     4,    -1,    -1,     7,     8,    -1,    10,
      -1,    12,    -1,    14,    15,    16,    17,    -1,    -1,    -1,
      21,    22,    -1,    24,    -1,    26,    27,    28,    -1,    30,
       3,     4,    -1,    -1,     7,     8,    -1,    10,    -1,    12,
      -1,    14,    15,    16,    17,    -1,    -1,    -1,    21,    22,
      -1,    24,    -1,    26,    27,    28,    -1,    30,     3,     4,
      -1,    -1,     7,     8,    -1,    10,    -1,    12,    -1,    14,
      15,    16,    17,    -1,    -1,    -1,    21,    22,    -1,    24,
      -1,    26,    -1,    28,     4,    30,    -1,     7,     8,    -1,
      10,    -1,    12,    -1,    14,    15,    16,    17,    -1,    -1,
      -1,    -1,    22,    -1,    24,    -1,    26,    -1,    28,     4,
      30,    -1,     7,     8,    -1,    10,    -1,    12,    -1,    14,
      15,    16,    17,    -1,    11,    -1,    -1,    22,    -1,    24,
      -1,    26,    19,    28,    11,    30,    -1,    -1,    25,    26,
      -1,    28,    19,    30,    11,    -1,    -1,    -1,    25,    26,
      -1,    28,    19,    30,    -1,    -1,    -1,    -1,    25,    26,
      -1,    28,    -1,    30
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     7,     8,    10,    12,    14,    15,    16,
      17,    21,    22,    24,    26,    28,    30,    32,    33,    34,
      35,    36,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    66,    26,    24,    57,
      39,    57,    65,    66,    25,    65,    17,    39,    52,    53,
      39,    39,     0,    33,    35,    34,    11,    19,    26,    28,
      30,     8,    13,    18,    24,    29,    29,     9,    10,     3,
      39,    39,    35,    35,    37,    27,     9,    21,    27,    33,
      39,    57,    39,    27,    35,    13,    39,    57,     8,    24,
      39,    39,    39,    39,    10,    39,    27,    25,    65,    66,
      65,    27,    17,    53,    10,    26,    27,    57,    39,    25,
      39,    39,    23,    25,    10,    39,    27,    35,    25,     5,
       6,    51,    39,    27,    26,    39,    39,    27,    39,    51
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
#line 107 "parser.y"
    { 
		parsing_result = (yyvsp[(1) - (1)].exp);
	;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 110 "parser.y"
    {
		empty* e=new_empty();
		ADD_DEBUG_INFO(e)
		parsing_result=(expression*)e;
	;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 122 "parser.y"
    {
		pointers_vector_push_ignore_duplicate(&((block*)(yyvsp[(1) - (3)].exp))->lines, (yyvsp[(3) - (3)].exp));
		(yyval.exp)=(yyvsp[(1) - (3)].exp);
	;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 126 "parser.y"
    {
		(yyval.exp)=(yyvsp[(2) - (2)].exp);
	;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 129 "parser.y"
    {
		(yyval.exp)=(yyvsp[(1) - (2)].exp);
	;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 132 "parser.y"
    {
		block* b=new_block();
		ADD_DEBUG_INFO(b)
		pointers_vector_push_ignore_duplicate(&b->lines, (yyvsp[(1) - (1)].exp));
		(yyval.exp)=(expression*)b;
	;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 140 "parser.y"
    { (yyval.exp)=(yyvsp[(3) - (5)].exp); ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 143 "parser.y"
    {
		(yyval.exp)=(yyvsp[(1) - (1)].exp); 
		(yyval.exp)->type=e_table_literal;
	;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 149 "parser.y"
    { 
		(yyval.exp)=(yyvsp[(3) - (5)].exp);
	;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 152 "parser.y"
    {
		block* b=new_block();
		ADD_DEBUG_INFO(b)
		b->type=e_table_literal;
		(yyval.exp)=(expression*)b;
	;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 179 "parser.y"
    {
		parentheses* p=new_parentheses();
		ADD_DEBUG_INFO(p)
		p->value=(yyvsp[(2) - (3)].exp);
		(yyval.exp)=(expression*)p;
	;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 196 "parser.y"
    {
		self_member_access* ma=new_self_member_access();
		ADD_DEBUG_INFO(ma)
		ma->right=(name*)(yyvsp[(2) - (2)].exp);
		(yyval.exp)=(expression*)ma;
	;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 204 "parser.y"
    {
		member_access* ma=new_member_access();
		ADD_DEBUG_INFO(ma)
		ma->left=(yyvsp[(1) - (3)].exp);
		ma->right=(name*)(yyvsp[(3) - (3)].exp);
		(yyval.exp)=(expression*)ma;
	;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 213 "parser.y"
    {
		null_conditional_member_access* ma=new_null_conditional_member_access();
		ADD_DEBUG_INFO(ma)
		ma->left=(yyvsp[(1) - (4)].exp);
		ma->right=(name*)(yyvsp[(4) - (4)].exp);
		(yyval.exp)=(expression*)ma;
	;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 222 "parser.y"
    {
		indexer* i=new_indexer();
		ADD_DEBUG_INFO(i)
		i->left=(yyvsp[(1) - (4)].exp);
		i->right=(yyvsp[(3) - (4)].exp);
		(yyval.exp)=(expression*)i;
	;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 231 "parser.y"
    {
		null_conditional_indexer* i=new_null_conditional_indexer();
		ADD_DEBUG_INFO(i)
		i->left=(yyvsp[(1) - (5)].exp);
		i->right=(yyvsp[(4) - (5)].exp);
		(yyval.exp)=(expression*)i;
	;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 240 "parser.y"
    {
		self_indexer* i=new_self_indexer();
		ADD_DEBUG_INFO(i)
		i->right=(yyvsp[(3) - (4)].exp);
		(yyval.exp)=(expression*)i;
	;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 248 "parser.y"
    {
		function_return* r=new_function_return();
		ADD_DEBUG_INFO(r)
		r->value=(yyvsp[(1) - (2)].exp);
		(yyval.exp)=(expression*)r;
	;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 256 "parser.y"
    {
		macro* m=new_macro();
		ADD_DEBUG_INFO(m)
		m->identifier=(name*)(yyvsp[(2) - (2)].exp);
		(yyval.exp)=(expression*)m;
	;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 264 "parser.y"
    {	
		conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[(3) - (5)].exp);
		c->ontrue=(yyvsp[(5) - (5)].exp);
		c->onfalse=(expression*)new_empty();
		(yyval.exp)=(expression*)c;
	;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 272 "parser.y"
    {
		conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[(3) - (6)].exp);
		c->ontrue=(yyvsp[(5) - (6)].exp);
		c->onfalse=(yyvsp[(6) - (6)].exp);
		(yyval.exp)=(expression*)c;
	;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 282 "parser.y"
    {
		(yyval.exp)=(yyvsp[(2) - (2)].exp)
	;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 285 "parser.y"
    {
		conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[(3) - (6)].exp);
		c->ontrue=(yyvsp[(5) - (6)].exp);
		c->onfalse=(yyvsp[(6) - (6)].exp);
		(yyval.exp)=(expression*)c;
	;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 293 "parser.y"
    {
		conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[(3) - (5)].exp);
		c->ontrue=(yyvsp[(5) - (5)].exp);
		c->onfalse=(expression*)new_empty();
		(yyval.exp)=(expression*)c;
	;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 303 "parser.y"
    {
		pointers_vector_push((yyvsp[(1) - (3)].args), (yyvsp[(3) - (3)].exp));
		(yyval.args)=(yyvsp[(1) - (3)].args);
	;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 307 "parser.y"
    {
		vector* args=malloc(sizeof(vector));
		CHECK_ALLOCATION(args);
		vector_init(args, sizeof(expression*), 4);
		pointers_vector_push(args, (yyvsp[(1) - (1)].exp));
		(yyval.args)=args;
	;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 316 "parser.y"
    {
		argument* a=new_argument();
		ADD_DEBUG_INFO(a)
		a->name=(yyvsp[(1) - (1)].sval);
		a->used_in_closure=false;
		(yyval.exp)=(expression*)a;
	;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 325 "parser.y"
    {
		function_declaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		vector_copy((yyvsp[(2) - (5)].args), &f->arguments);
		vector_deinit((yyvsp[(2) - (5)].args));
		free((yyvsp[(2) - (5)].args));
		f->variadic=false;
		f->body=(yyvsp[(5) - (5)].exp);
		(yyval.exp)=(expression*)f;
	;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 335 "parser.y"
    {
		function_declaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		vector_copy((yyvsp[(2) - (6)].args), &f->arguments);
		vector_deinit((yyvsp[(2) - (6)].args));
		free((yyvsp[(2) - (6)].args));
		f->variadic=true;
		f->body=(yyvsp[(6) - (6)].exp);
		(yyval.exp)=(expression*)f;
	;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 345 "parser.y"
    {
		function_declaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		pointers_vector_push(&f->arguments, name_to_argument((name*)(yyvsp[(1) - (4)].exp)));
		f->variadic=true;
		f->body=(yyvsp[(4) - (4)].exp);
		(yyval.exp)=(expression*)f;
	;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 353 "parser.y"
    {
		function_declaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		pointers_vector_push(&f->arguments, name_to_argument((name*)(yyvsp[(1) - (3)].exp)));
		f->variadic=false;
		f->body=(yyvsp[(3) - (3)].exp);
		(yyval.exp)=(expression*)f;
	;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 361 "parser.y"
    {
		function_declaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		f->variadic=false;
		f->body=(yyvsp[(2) - (2)].exp);
		(yyval.exp)=(expression*)f;
	;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 370 "parser.y"
    {
		int_literal* l=new_int_literal();
		ADD_DEBUG_INFO(l)
		l->value=(yyvsp[(1) - (1)].ival);
		(yyval.exp)=(expression*)l;
	;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 376 "parser.y"
    { 
		float_literal* l=new_float_literal();
		ADD_DEBUG_INFO(l)
		l->value=(yyvsp[(1) - (1)].fval);
		(yyval.exp)=(expression*)l;
	;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 382 "parser.y"
    { 
		string_literal* l=new_string_literal();
		ADD_DEBUG_INFO(l)
		l->value=(yyvsp[(1) - (1)].sval);
		(yyval.exp)=(expression*)l;
	;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 390 "parser.y"
    {
		empty* e=new_empty();
		ADD_DEBUG_INFO(e)
		(yyval.exp)=(expression*)e;
	;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 397 "parser.y"
    {
		name* n=new_name();
		ADD_DEBUG_INFO(n)
		n->value=(yyvsp[(1) - (1)].sval);
		(yyval.exp)=(expression*)n;
	  ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 406 "parser.y"
    {
		assignment* a=new_assignment();
		ADD_DEBUG_INFO(a)
		a->left=(expression*)(yyvsp[(1) - (3)].exp);
		binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=copy_expression((yyvsp[(1) - (3)].exp));
		u->op=(yyvsp[(2) - (3)].sval);
		u->right=(yyvsp[(3) - (3)].exp);
		a->right=(expression*)u;
		a->used_in_closure=false;
		(yyval.exp)=(expression*)a;
	;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 420 "parser.y"
    {
		assignment* a=new_assignment();
		ADD_DEBUG_INFO(a)
		a->left=(expression*)(yyvsp[(1) - (3)].exp);
		a->right=(yyvsp[(3) - (3)].exp);
		a->used_in_closure=false;
		(yyval.exp)=(expression*)a;
	;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 431 "parser.y"
    {
		macro_declaration* md=new_macro_declaration();
		ADD_DEBUG_INFO(md)
		md->left=(macro*)(yyvsp[(1) - (3)].exp);
		md->right=(yyvsp[(3) - (3)].exp);
		(yyval.exp)=(expression*)md;
	;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 440 "parser.y"
    {
		function_call* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->called=(yyvsp[(1) - (4)].exp);
		c->arguments=(table_literal*)(yyvsp[(3) - (4)].exp);
		c->arguments->type=e_table_literal;
		(yyval.exp)=(expression*)c;
	;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 448 "parser.y"
    {
		function_call* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->called=(yyvsp[(1) - (3)].exp);
		c->arguments=(table_literal*)new_block();
		(yyval.exp)=(expression*)c;
	;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 457 "parser.y"
    {
		message* m=new_message();
		ADD_DEBUG_INFO(m)
		m->messaged_object=(yyvsp[(1) - (6)].exp);
		m->message_name=(name*)(yyvsp[(3) - (6)].exp);
		m->arguments=(table_literal*)(yyvsp[(5) - (6)].exp);
		m->arguments->type=e_table_literal;
		(yyval.exp)=(expression*)m;
	;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 466 "parser.y"
    {
		message* m=new_message();
		ADD_DEBUG_INFO(m)
		m->messaged_object=(yyvsp[(1) - (5)].exp);
		m->message_name=(name*)(yyvsp[(3) - (5)].exp);
		m->arguments=(table_literal*)new_block();
		(yyval.exp)=(expression*)m;
	;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 477 "parser.y"
    {
		binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=(yyvsp[(1) - (3)].exp);
		u->op=(yyvsp[(2) - (3)].sval);
		u->right=(yyvsp[(3) - (3)].exp);
		(yyval.exp)=(expression*)u;
	;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 486 "parser.y"
    {
		binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=(yyvsp[(1) - (3)].exp);
		u->op=strdup("-");
		u->right=(yyvsp[(3) - (3)].exp);
		(yyval.exp)=(expression*)u;
	;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 497 "parser.y"
    {
		prefix* p=new_prefix();
		ADD_DEBUG_INFO(p)
		p->op=strdup("!");
		p->right=(yyvsp[(2) - (2)].exp);
		(yyval.exp)=(expression*)p;
	;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 505 "parser.y"
    {
		prefix* p=new_prefix();
		ADD_DEBUG_INFO(p)
		p->op=strdup("-");
		p->right=(yyvsp[(2) - (2)].exp);
		(yyval.exp)=(expression*)p;
	;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 515 "parser.y"
    {
		return_if_error* r=new_return_if_error();
		ADD_DEBUG_INFO(r)
		r->value=(yyvsp[(1) - (3)].exp);
		(yyval.exp)=(expression*)r;
	;}
    break;



/* Line 1455 of yacc.c  */
#line 2164 "parser.tab.c"
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
#line 528 "parser.y"


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
	if(parsing_result!=NULL){
		postprocess_ast(&parsing_result);
	}
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
