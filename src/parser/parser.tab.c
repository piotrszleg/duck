/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 1 "parser.y" /* yacc.c:339  */

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

#line 104 "parser.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "parser.tab.h".  */
#ifndef YY_YY_PARSER_TAB_H_INCLUDED
# define YY_YY_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
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

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 44 "parser.y" /* yacc.c:355  */

	int ival;
	float fval;
	char *sval;
	struct vector* args;
	struct Expression* expression;

#line 173 "parser.tab.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_PARSER_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 190 "parser.tab.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  62
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   397

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  31
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  36
/* YYNRULES -- Number of rules.  */
#define YYNRULES  83
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  140

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   275

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
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
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
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

#if YYDEBUG || YYERROR_VERBOSE || 0
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
  "literal", "null_literal", "name", "assignment", "macro_declaration",
  "call", "message", "binary", "prefix", "return_if_error", "OPT_ENDLS",
  "ENDLS", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,    44,   123,   125,    91,    93,    40,    41,    33,    61,
      45
};
# endif

#define YYPACT_NINF -55

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-55)))

#define YYTABLE_NINF -58

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     100,   -55,    -8,   -55,    23,   239,     6,   -55,   -55,   -55,
     -55,   -55,    18,    11,   264,   239,   239,    22,   -55,   214,
       5,   -55,   -55,   367,   -55,    -2,   -55,   -55,   -55,   -55,
     -55,   -55,   -55,    16,   -55,   -55,   -55,   -55,    10,   -55,
     -55,   -55,   -55,   -55,   -55,   -55,    29,   239,   239,   -55,
     367,   -55,   214,    29,   -55,   214,     8,   274,    69,   -55,
     367,   367,   -55,   -55,     5,   130,     6,     6,    25,   239,
     239,   158,    15,   239,   239,   239,   239,    40,   239,   -55,
     295,   305,     5,     5,    18,   -55,    27,    38,    46,   -55,
     367,   -55,    32,     6,   239,   367,   326,   -55,     3,   -55,
     367,   367,   367,   367,   239,   367,   239,   -55,    39,    29,
      52,    70,   -55,   -55,   239,   186,   -55,   336,   -55,   -55,
     367,    33,   -55,   -55,   239,   367,   -55,     4,   -55,    53,
     239,   -55,   367,   -55,   239,   367,   357,   239,    33,   -55
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      16,    83,     0,    66,     0,    16,     0,    63,    64,    65,
      67,     5,    81,    81,    16,    16,    16,     0,     6,    16,
       2,    19,    20,    11,    17,    21,    36,    37,    40,    39,
      41,    38,    32,    30,    25,    24,    18,    28,    35,    22,
      31,    23,    29,    26,    27,    33,     4,    16,    16,    42,
      62,    49,    16,    80,    15,    16,    67,     0,     0,    56,
      77,    78,     1,     6,     9,    10,     0,     0,     0,    16,
      16,    16,    48,    16,    16,    16,    16,     0,    16,    82,
       0,     0,    81,    13,    81,    34,     0,     0,     0,     7,
       8,    43,     0,     0,    16,    75,     0,    72,     0,    79,
      76,    68,    69,    70,    16,    61,    16,    47,     0,     4,
       0,     0,    57,    55,    16,    16,    44,     0,    45,    71,
      60,    50,    12,    14,    16,    58,    74,     0,    46,     0,
      16,    51,    59,    73,    16,    52,     0,    16,    54,    53
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -55,   -55,   -14,   -16,   -18,   -55,   -55,   -55,    -5,   -55,
     -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,
     -54,   -55,     0,   -55,   -55,   -55,     9,   -55,   -55,   -55,
     -55,   -55,   -55,   -55,    -1,   -10
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    17,    18,    19,    20,    21,    84,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
     131,    58,    59,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    52,    46
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      50,    64,    53,    53,    65,    63,     1,     1,     1,    57,
      60,    61,    55,    49,     1,    51,    74,   -57,    47,    77,
      78,     1,    62,    10,    11,    11,    11,    75,    99,   -57,
     119,   133,    79,    93,    82,   -57,    54,    83,   129,   130,
      10,    66,    80,    81,    67,    76,    68,    48,    65,    94,
     104,    89,    69,    98,   111,   112,   114,    70,   115,    71,
      90,    72,   122,    73,    95,    96,    65,    65,   100,   101,
     102,   103,   109,   105,    53,    91,    92,   123,    86,   134,
     124,   108,    65,   110,   139,     0,     0,   113,     0,   117,
      87,     0,     0,     0,     0,     0,    88,   127,     0,   120,
      -3,   121,   116,     1,     2,     0,     0,     3,     4,   125,
       5,    65,     6,     0,     7,     8,     9,    10,     0,   132,
       0,    11,    12,     0,    13,   135,    14,     0,    15,   136,
      16,     0,   138,     1,     2,     0,     0,     3,     4,     0,
       5,   -16,     6,   -16,     7,     8,     9,    10,     0,   -16,
       0,    11,    12,     0,    13,     0,    14,     0,    15,     0,
      16,     1,     2,     0,     0,     3,     4,     0,     5,     0,
       6,     0,     7,     8,     9,    10,     0,     0,     0,    11,
      12,     0,    13,     0,    14,    97,    15,     0,    16,     1,
       2,     0,     0,     3,     4,     0,     5,     0,     6,     0,
       7,     8,     9,    10,     0,     0,     0,    11,    12,     0,
      13,     0,    14,   126,    15,     0,    16,     1,     2,     0,
       0,     3,     4,     0,     5,     0,     6,     0,     7,     8,
       9,    10,     0,     0,     0,    11,    12,     0,    13,     0,
      14,     0,    15,     2,    16,     0,     3,     4,     0,     5,
       0,     6,     0,     7,     8,     9,    10,     0,     0,     0,
       0,    12,     0,    13,     0,    14,     0,    15,     2,    16,
       0,     3,     4,     0,     5,     0,     6,     0,     7,     8,
       9,    56,    66,     0,     0,    67,    12,    68,    13,     0,
      14,     0,    15,    69,    16,     0,     0,     0,    70,     0,
      71,    85,    72,    66,    73,     0,    67,     0,    68,     0,
       0,     0,     0,    66,    69,     0,    67,     0,    68,    70,
       0,    71,   106,    72,    69,    73,     0,     0,     0,    70,
     107,    71,     0,    72,    66,    73,     0,    67,     0,    68,
       0,     0,     0,     0,    66,    69,     0,    67,     0,    68,
      70,   118,    71,     0,    72,    69,    73,     0,     0,     0,
      70,   128,    71,     0,    72,    66,    73,     0,    67,     0,
      68,     0,     0,     0,     0,    66,    69,     0,    67,     0,
      68,    70,     0,    71,   137,    72,    69,    73,     0,     0,
       0,    70,     0,    71,     0,    72,     0,    73
};

static const yytype_int16 yycheck[] =
{
       5,    19,    12,    13,    20,    19,     3,     3,     3,    14,
      15,    16,    13,     4,     3,     6,    18,     9,    26,     9,
      10,     3,     0,    17,    21,    21,    21,    29,    13,    21,
      27,    27,     3,     8,    52,    27,    25,    55,     5,     6,
      17,     8,    47,    48,    11,    29,    13,    24,    64,    24,
      10,    65,    19,    71,    27,    17,    10,    24,    26,    26,
      65,    28,    23,    30,    69,    70,    82,    83,    73,    74,
      75,    76,    82,    78,    84,    66,    67,    25,     9,    26,
      10,    82,    98,    84,   138,    -1,    -1,    87,    -1,    94,
      21,    -1,    -1,    -1,    -1,    -1,    27,   115,    -1,   104,
       0,   106,    93,     3,     4,    -1,    -1,     7,     8,   114,
      10,   127,    12,    -1,    14,    15,    16,    17,    -1,   124,
      -1,    21,    22,    -1,    24,   130,    26,    -1,    28,   134,
      30,    -1,   137,     3,     4,    -1,    -1,     7,     8,    -1,
      10,    11,    12,    13,    14,    15,    16,    17,    -1,    19,
      -1,    21,    22,    -1,    24,    -1,    26,    -1,    28,    -1,
      30,     3,     4,    -1,    -1,     7,     8,    -1,    10,    -1,
      12,    -1,    14,    15,    16,    17,    -1,    -1,    -1,    21,
      22,    -1,    24,    -1,    26,    27,    28,    -1,    30,     3,
       4,    -1,    -1,     7,     8,    -1,    10,    -1,    12,    -1,
      14,    15,    16,    17,    -1,    -1,    -1,    21,    22,    -1,
      24,    -1,    26,    27,    28,    -1,    30,     3,     4,    -1,
      -1,     7,     8,    -1,    10,    -1,    12,    -1,    14,    15,
      16,    17,    -1,    -1,    -1,    21,    22,    -1,    24,    -1,
      26,    -1,    28,     4,    30,    -1,     7,     8,    -1,    10,
      -1,    12,    -1,    14,    15,    16,    17,    -1,    -1,    -1,
      -1,    22,    -1,    24,    -1,    26,    -1,    28,     4,    30,
      -1,     7,     8,    -1,    10,    -1,    12,    -1,    14,    15,
      16,    17,     8,    -1,    -1,    11,    22,    13,    24,    -1,
      26,    -1,    28,    19,    30,    -1,    -1,    -1,    24,    -1,
      26,    27,    28,     8,    30,    -1,    11,    -1,    13,    -1,
      -1,    -1,    -1,     8,    19,    -1,    11,    -1,    13,    24,
      -1,    26,    27,    28,    19,    30,    -1,    -1,    -1,    24,
      25,    26,    -1,    28,     8,    30,    -1,    11,    -1,    13,
      -1,    -1,    -1,    -1,     8,    19,    -1,    11,    -1,    13,
      24,    25,    26,    -1,    28,    19,    30,    -1,    -1,    -1,
      24,    25,    26,    -1,    28,     8,    30,    -1,    11,    -1,
      13,    -1,    -1,    -1,    -1,     8,    19,    -1,    11,    -1,
      13,    24,    -1,    26,    27,    28,    19,    30,    -1,    -1,
      -1,    24,    -1,    26,    -1,    28,    -1,    30
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
      39,    39,     0,    33,    35,    34,     8,    11,    13,    19,
      24,    26,    28,    30,    18,    29,    29,     9,    10,     3,
      39,    39,    35,    35,    37,    27,     9,    21,    27,    33,
      39,    57,    57,     8,    24,    39,    39,    27,    35,    13,
      39,    39,    39,    39,    10,    39,    27,    25,    65,    66,
      65,    27,    17,    53,    10,    26,    57,    39,    25,    27,
      39,    39,    23,    25,    10,    39,    27,    35,    25,     5,
       6,    51,    39,    27,    26,    39,    39,    27,    39,    51
};

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

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
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


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
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
  int yytoken = 0;
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

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
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
      if (yytable_value_is_error (yyn))
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
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

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
#line 107 "parser.y" /* yacc.c:1646  */
    { 
		parsing_result = (yyvsp[0].expression);
	}
#line 1420 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 110 "parser.y" /* yacc.c:1646  */
    {
		Empty* e=new_empty();
		ADD_DEBUG_INFO(e)
		parsing_result=(Expression*)e;
	}
#line 1430 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 122 "parser.y" /* yacc.c:1646  */
    {
		pointers_vector_push_ignore_duplicate(&((Block*)(yyvsp[-2].expression))->lines, (yyvsp[0].expression));
		(yyval.expression)=(yyvsp[-2].expression);
	}
#line 1439 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 126 "parser.y" /* yacc.c:1646  */
    {
		(yyval.expression)=(yyvsp[0].expression);
	}
#line 1447 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 129 "parser.y" /* yacc.c:1646  */
    {
		(yyval.expression)=(yyvsp[-1].expression);
	}
#line 1455 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 132 "parser.y" /* yacc.c:1646  */
    {
		Block* b=new_block();
		ADD_DEBUG_INFO(b)
		pointers_vector_push_ignore_duplicate(&b->lines, (yyvsp[0].expression));
		(yyval.expression)=(Expression*)b;
	}
#line 1466 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 140 "parser.y" /* yacc.c:1646  */
    { (yyval.expression)=(yyvsp[-2].expression); }
#line 1472 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 143 "parser.y" /* yacc.c:1646  */
    {
		(yyval.expression)=(yyvsp[0].expression); 
		(yyval.expression)->type=e_table_literal;
	}
#line 1481 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 149 "parser.y" /* yacc.c:1646  */
    { 
		(yyval.expression)=(yyvsp[-2].expression);
	}
#line 1489 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 152 "parser.y" /* yacc.c:1646  */
    {
		Block* b=new_block();
		ADD_DEBUG_INFO(b)
		b->type=e_table_literal;
		(yyval.expression)=(Expression*)b;
	}
#line 1500 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 179 "parser.y" /* yacc.c:1646  */
    {
		Parentheses* p=new_parentheses();
		ADD_DEBUG_INFO(p)
		p->value=(yyvsp[-1].expression);
		(yyval.expression)=(Expression*)p;
	}
#line 1511 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 196 "parser.y" /* yacc.c:1646  */
    {
		SelfMemberAccess* ma=new_self_member_access();
		ADD_DEBUG_INFO(ma)
		ma->right=(Name*)(yyvsp[0].expression);
		(yyval.expression)=(Expression*)ma;
	}
#line 1522 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 204 "parser.y" /* yacc.c:1646  */
    {
		MemberAccess* ma=new_member_access();
		ADD_DEBUG_INFO(ma)
		ma->left=(yyvsp[-2].expression);
		ma->right=(Name*)(yyvsp[0].expression);
		(yyval.expression)=(Expression*)ma;
	}
#line 1534 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 213 "parser.y" /* yacc.c:1646  */
    {
		NullConditionalMemberAccess* ma=new_null_conditional_member_access();
		ADD_DEBUG_INFO(ma)
		ma->left=(yyvsp[-3].expression);
		ma->right=(Name*)(yyvsp[0].expression);
		(yyval.expression)=(Expression*)ma;
	}
#line 1546 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 222 "parser.y" /* yacc.c:1646  */
    {
		Indexer* i=new_indexer();
		ADD_DEBUG_INFO(i)
		i->left=(yyvsp[-3].expression);
		i->right=(yyvsp[-1].expression);
		(yyval.expression)=(Expression*)i;
	}
#line 1558 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 231 "parser.y" /* yacc.c:1646  */
    {
		NullConditionalIndexer* i=new_null_conditional_indexer();
		ADD_DEBUG_INFO(i)
		i->left=(yyvsp[-4].expression);
		i->right=(yyvsp[-1].expression);
		(yyval.expression)=(Expression*)i;
	}
#line 1570 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 240 "parser.y" /* yacc.c:1646  */
    {
		SelfIndexer* i=new_self_indexer();
		ADD_DEBUG_INFO(i)
		i->right=(yyvsp[-1].expression);
		(yyval.expression)=(Expression*)i;
	}
#line 1581 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 248 "parser.y" /* yacc.c:1646  */
    {
		FunctionReturn* r=new_function_return();
		ADD_DEBUG_INFO(r)
		r->value=(yyvsp[-1].expression);
		(yyval.expression)=(Expression*)r;
	}
#line 1592 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 256 "parser.y" /* yacc.c:1646  */
    {
		Macro* m=new_macro();
		ADD_DEBUG_INFO(m)
		m->identifier=(Name*)(yyvsp[0].expression);
		(yyval.expression)=(Expression*)m;
	}
#line 1603 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 264 "parser.y" /* yacc.c:1646  */
    {	
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[-2].expression);
		c->ontrue=(yyvsp[0].expression);
		c->onfalse=(Expression*)new_empty();
		(yyval.expression)=(Expression*)c;
	}
#line 1616 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 272 "parser.y" /* yacc.c:1646  */
    {
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[-3].expression);
		c->ontrue=(yyvsp[-1].expression);
		c->onfalse=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)c;
	}
#line 1629 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 282 "parser.y" /* yacc.c:1646  */
    {
		(yyval.expression)=(yyvsp[0].expression);
	}
#line 1637 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 285 "parser.y" /* yacc.c:1646  */
    {
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[-3].expression);
		c->ontrue=(yyvsp[-1].expression);
		c->onfalse=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)c;
	}
#line 1650 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 293 "parser.y" /* yacc.c:1646  */
    {
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[-2].expression);
		c->ontrue=(yyvsp[0].expression);
		c->onfalse=(Expression*)new_empty();
		(yyval.expression)=(Expression*)c;
	}
#line 1663 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 303 "parser.y" /* yacc.c:1646  */
    {
		pointers_vector_push((yyvsp[-2].args), (yyvsp[0].expression));
		(yyval.args)=(yyvsp[-2].args);
	}
#line 1672 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 307 "parser.y" /* yacc.c:1646  */
    {
		vector* args=malloc(sizeof(vector));
		CHECK_ALLOCATION(args);
		vector_init(args, sizeof(Expression*), 4);
		pointers_vector_push(args, (yyvsp[0].expression));
		(yyval.args)=args;
	}
#line 1684 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 316 "parser.y" /* yacc.c:1646  */
    {
		Argument* a=new_argument();
		ADD_DEBUG_INFO(a)
		a->name=(yyvsp[0].sval);
		a->used_in_closure=false;
		(yyval.expression)=(Expression*)a;
	}
#line 1696 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 325 "parser.y" /* yacc.c:1646  */
    {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		vector_copy((yyvsp[-3].args), &f->arguments);
		vector_deinit((yyvsp[-3].args));
		free((yyvsp[-3].args));
		f->variadic=false;
		f->body=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)f;
	}
#line 1711 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 335 "parser.y" /* yacc.c:1646  */
    {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		vector_copy((yyvsp[-4].args), &f->arguments);
		vector_deinit((yyvsp[-4].args));
		free((yyvsp[-4].args));
		f->variadic=true;
		f->body=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)f;
	}
#line 1726 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 345 "parser.y" /* yacc.c:1646  */
    {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		pointers_vector_push(&f->arguments, name_to_argument((Name*)(yyvsp[-3].expression)));
		f->variadic=true;
		f->body=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)f;
	}
#line 1739 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 353 "parser.y" /* yacc.c:1646  */
    {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		pointers_vector_push(&f->arguments, name_to_argument((Name*)(yyvsp[-2].expression)));
		f->variadic=false;
		f->body=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)f;
	}
#line 1752 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 361 "parser.y" /* yacc.c:1646  */
    {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		f->variadic=false;
		f->body=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)f;
	}
#line 1764 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 370 "parser.y" /* yacc.c:1646  */
    {
		IntLiteral* l=new_int_literal();
		ADD_DEBUG_INFO(l)
		l->value=(yyvsp[0].ival);
		(yyval.expression)=(Expression*)l;
	}
#line 1775 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 376 "parser.y" /* yacc.c:1646  */
    { 
		FloatLiteral* l=new_float_literal();
		ADD_DEBUG_INFO(l)
		l->value=(yyvsp[0].fval);
		(yyval.expression)=(Expression*)l;
	}
#line 1786 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 382 "parser.y" /* yacc.c:1646  */
    { 
		StringLiteral* l=new_string_literal();
		ADD_DEBUG_INFO(l)
		l->value=(yyvsp[0].sval);
		(yyval.expression)=(Expression*)l;
	}
#line 1797 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 390 "parser.y" /* yacc.c:1646  */
    {
		NullLiteral* n=new_null_literal();
		ADD_DEBUG_INFO(n)
		(yyval.expression)=(Expression*)n;
	}
#line 1807 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 397 "parser.y" /* yacc.c:1646  */
    {
		Name* n=new_name();
		ADD_DEBUG_INFO(n)
		n->value=(yyvsp[0].sval);
		(yyval.expression)=(Expression*)n;
	  }
#line 1818 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 406 "parser.y" /* yacc.c:1646  */
    {
		Assignment* a=new_assignment();
		ADD_DEBUG_INFO(a)
		a->left=(Expression*)(yyvsp[-2].expression);
		Binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=copy_expression((yyvsp[-2].expression));
		u->op=(yyvsp[-1].sval);
		u->right=(yyvsp[0].expression);
		a->right=(Expression*)u;
		a->used_in_closure=false;
		(yyval.expression)=(Expression*)a;
	}
#line 1836 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 420 "parser.y" /* yacc.c:1646  */
    {
		Assignment* a=new_assignment();
		ADD_DEBUG_INFO(a)
		a->left=(Expression*)(yyvsp[-2].expression);
		a->right=(yyvsp[0].expression);
		a->used_in_closure=false;
		(yyval.expression)=(Expression*)a;
	}
#line 1849 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 431 "parser.y" /* yacc.c:1646  */
    {
		MacroDeclaration* md=new_macro_declaration();
		ADD_DEBUG_INFO(md)
		md->left=(Macro*)(yyvsp[-2].expression);
		md->right=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)md;
	}
#line 1861 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 440 "parser.y" /* yacc.c:1646  */
    {
		FunctionCall* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->called=(yyvsp[-3].expression);
		c->arguments=(TableLiteral*)(yyvsp[-1].expression);
		c->arguments->type=e_table_literal;
		(yyval.expression)=(Expression*)c;
	}
#line 1874 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 448 "parser.y" /* yacc.c:1646  */
    {
		FunctionCall* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->called=(yyvsp[-2].expression);
		c->arguments=(TableLiteral*)new_block();
		(yyval.expression)=(Expression*)c;
	}
#line 1886 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 457 "parser.y" /* yacc.c:1646  */
    {
		Message* m=new_message();
		ADD_DEBUG_INFO(m)
		m->messaged_object=(yyvsp[-5].expression);
		m->message_name=(Name*)(yyvsp[-3].expression);
		m->arguments=(TableLiteral*)(yyvsp[-1].expression);
		m->arguments->type=e_table_literal;
		(yyval.expression)=(Expression*)m;
	}
#line 1900 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 466 "parser.y" /* yacc.c:1646  */
    {
		Message* m=new_message();
		ADD_DEBUG_INFO(m)
		m->messaged_object=(yyvsp[-4].expression);
		m->message_name=(Name*)(yyvsp[-2].expression);
		m->arguments=(TableLiteral*)new_block();
		(yyval.expression)=(Expression*)m;
	}
#line 1913 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 477 "parser.y" /* yacc.c:1646  */
    {
		Binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=(yyvsp[-2].expression);
		u->op=(yyvsp[-1].sval);
		u->right=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)u;
	}
#line 1926 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 486 "parser.y" /* yacc.c:1646  */
    {
		Binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=(yyvsp[-2].expression);
		u->op=strdup("-");
		u->right=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)u;
	}
#line 1939 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 497 "parser.y" /* yacc.c:1646  */
    {
		Prefix* p=new_prefix();
		ADD_DEBUG_INFO(p)
		p->op=strdup("!");
		p->right=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)p;
	}
#line 1951 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 505 "parser.y" /* yacc.c:1646  */
    {
		Prefix* p=new_prefix();
		ADD_DEBUG_INFO(p)
		p->op=strdup("-");
		p->right=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)p;
	}
#line 1963 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 515 "parser.y" /* yacc.c:1646  */
    {
		ReturnIfError* r=new_return_if_error();
		ADD_DEBUG_INFO(r)
		r->value=(yyvsp[-2].expression);
		(yyval.expression)=(Expression*)r;
	}
#line 1974 "parser.tab.c" /* yacc.c:1646  */
    break;


#line 1978 "parser.tab.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
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
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
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
  return yyresult;
}
#line 528 "parser.y" /* yacc.c:1906  */


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
