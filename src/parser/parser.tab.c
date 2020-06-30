/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.5.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
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
				function->optional_arguments_count++;
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

#line 189 "parser.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
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
#line 125 "parser.y"

	int ival;
	float fval;
	char *sval;
	struct vector* arguments;
	struct Expression* expression;

#line 270 "parser.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_PARSER_TAB_H_INCLUDED  */



#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))

/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

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

#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
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

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

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
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
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
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

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
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  145

#define YYUNDEFTOK  2
#define YYMAXUTOK   275


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
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
static const yytype_int16 yyrline[] =
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
  "table_contents", "table", "expression", "parentheses", "unpack",
  "identifier", "self_member_access", "member_access",
  "null_conditional_member_access", "indexer", "null_conditional_indexer",
  "self_indexer", "return", "macro", "conditional", "conditional_else",
  "arguments", "argument", "variadic_argument", "optional_argument",
  "function", "literal", "null_literal", "name", "assignment",
  "macro_declaration", "call", "message", "binary", "prefix",
  "return_if_error", "OPT_ENDLS", "ENDLS", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,    44,   123,   125,    91,    93,    40,    41,    33,    61,
      45
};
# endif

#define YYPACT_NINF (-60)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-60)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
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

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
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

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -60,   -60,   -14,   -16,   -18,   -60,   -60,   -60,    -5,   -60,
     -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,
     -60,   -59,   -60,    -4,   -60,   -60,   -60,   -60,   -60,     9,
     -60,   -60,   -60,   -60,   -60,   -60,   -60,     3,   -10
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    18,    19,    20,    21,    22,    89,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,   137,    60,    61,    62,    63,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    55,    48
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
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
static const yytype_int8 yystos[] =
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

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
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

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
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


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
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


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[+yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
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
#   define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#  else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
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
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
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
            else
              goto append;

          append:
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

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
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
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                yy_state_t *yyssp, int yytoken)
{
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Actual size of YYARG. */
  int yycount = 0;
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

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
      int yyn = yypact[+*yyssp];
      YYPTRDIFF_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
      yysize = yysize0;
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
                  YYPTRDIFF_T yysize1
                    = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
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
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    /* Don't count the "%s"s in the final size, but reserve room for
       the terminator.  */
    YYPTRDIFF_T yysize1 = yysize + (yystrlen (yyformat) - 2 * yycount) + 1;
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
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
          ++yyp;
          ++yyformat;
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
    yy_state_fast_t yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss;
    yy_state_t *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYPTRDIFF_T yystacksize;

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
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
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
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

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
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
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
#line 191 "parser.y"
              { 
		parsing_result = (yyvsp[0].expression);
	}
#line 1601 "parser.tab.c"
    break;

  case 3:
#line 194 "parser.y"
          {
		Empty* e=new_empty();
		ADD_DEBUG_INFO(e)
		parsing_result=(Expression*)e;
	}
#line 1611 "parser.tab.c"
    break;

  case 8:
#line 206 "parser.y"
                                          {
		pointers_vector_push_ignore_duplicate(&((Block*)(yyvsp[-2].expression))->lines, (yyvsp[0].expression));
		(yyval.expression)=(yyvsp[-2].expression);
	}
#line 1620 "parser.tab.c"
    break;

  case 9:
#line 210 "parser.y"
                                 {
		(yyval.expression)=(yyvsp[0].expression);
	}
#line 1628 "parser.tab.c"
    break;

  case 10:
#line 213 "parser.y"
                                 {
		(yyval.expression)=(yyvsp[-1].expression);
	}
#line 1636 "parser.tab.c"
    break;

  case 11:
#line 216 "parser.y"
                     {
		Block* b=new_block();
		ADD_DEBUG_INFO(b)
		pointers_vector_push_ignore_duplicate(&b->lines, (yyvsp[0].expression));
		(yyval.expression)=(Expression*)b;
	}
#line 1647 "parser.tab.c"
    break;

  case 12:
#line 224 "parser.y"
                                          { (yyval.expression)=(yyvsp[-2].expression); }
#line 1653 "parser.tab.c"
    break;

  case 13:
#line 227 "parser.y"
              {
		(yyval.expression)=(yyvsp[0].expression); 
		(yyval.expression)->type=e_table_literal;
	}
#line 1662 "parser.tab.c"
    break;

  case 14:
#line 233 "parser.y"
                                                   { 
		(yyval.expression)=(yyvsp[-2].expression);
	}
#line 1670 "parser.tab.c"
    break;

  case 15:
#line 236 "parser.y"
                  {
		Block* b=new_block();
		ADD_DEBUG_INFO(b)
		b->type=e_table_literal;
		(yyval.expression)=(Expression*)b;
	}
#line 1681 "parser.tab.c"
    break;

  case 35:
#line 264 "parser.y"
                           {
		Parentheses* p=new_parentheses();
		ADD_DEBUG_INFO(p)
		p->value=(yyvsp[-1].expression);
		(yyval.expression)=(Expression*)p;
	}
#line 1692 "parser.tab.c"
    break;

  case 36:
#line 272 "parser.y"
                            {
		Unpack* u=new_unpack();
		ADD_DEBUG_INFO(u)
		u->value=(Expression*)(yyvsp[0].expression);
		(yyval.expression)=(Expression*)u;
	}
#line 1703 "parser.tab.c"
    break;

  case 44:
#line 289 "parser.y"
                 {
		SelfMemberAccess* ma=new_self_member_access();
		ADD_DEBUG_INFO(ma)
		ma->right=(Name*)(yyvsp[0].expression);
		(yyval.expression)=(Expression*)ma;
	}
#line 1714 "parser.tab.c"
    break;

  case 45:
#line 297 "parser.y"
                            {
		MemberAccess* ma=new_member_access();
		ADD_DEBUG_INFO(ma)
		ma->left=(yyvsp[-2].expression);
		ma->right=(Name*)(yyvsp[0].expression);
		(yyval.expression)=(Expression*)ma;
	}
#line 1726 "parser.tab.c"
    break;

  case 46:
#line 306 "parser.y"
                                          {
		NullConditionalMemberAccess* ma=new_null_conditional_member_access();
		ADD_DEBUG_INFO(ma)
		ma->left=(yyvsp[-3].expression);
		ma->right=(Name*)(yyvsp[0].expression);
		(yyval.expression)=(Expression*)ma;
	}
#line 1738 "parser.tab.c"
    break;

  case 47:
#line 315 "parser.y"
                                      {
		Indexer* i=new_indexer();
		ADD_DEBUG_INFO(i)
		i->left=(yyvsp[-3].expression);
		i->right=(yyvsp[-1].expression);
		(yyval.expression)=(Expression*)i;
	}
#line 1750 "parser.tab.c"
    break;

  case 48:
#line 324 "parser.y"
                                                    {
		NullConditionalIndexer* i=new_null_conditional_indexer();
		ADD_DEBUG_INFO(i)
		i->left=(yyvsp[-4].expression);
		i->right=(yyvsp[-1].expression);
		(yyval.expression)=(Expression*)i;
	}
#line 1762 "parser.tab.c"
    break;

  case 49:
#line 333 "parser.y"
                               {
		SelfIndexer* i=new_self_indexer();
		ADD_DEBUG_INFO(i)
		i->right=(yyvsp[-1].expression);
		(yyval.expression)=(Expression*)i;
	}
#line 1773 "parser.tab.c"
    break;

  case 50:
#line 341 "parser.y"
                       {
		FunctionReturn* r=new_function_return();
		ADD_DEBUG_INFO(r)
		r->value=(yyvsp[-1].expression);
		(yyval.expression)=(Expression*)r;
	}
#line 1784 "parser.tab.c"
    break;

  case 51:
#line 349 "parser.y"
                {
		Macro* m=new_macro();
		ADD_DEBUG_INFO(m)
		m->identifier=(Name*)(yyvsp[0].expression);
		(yyval.expression)=(Expression*)m;
	}
#line 1795 "parser.tab.c"
    break;

  case 52:
#line 357 "parser.y"
                                          {	
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[-2].expression);
		c->ontrue=(yyvsp[0].expression);
		c->onfalse=(Expression*)new_empty();
		(yyval.expression)=(Expression*)c;
	}
#line 1808 "parser.tab.c"
    break;

  case 53:
#line 365 "parser.y"
                                                            {
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[-3].expression);
		c->ontrue=(yyvsp[-1].expression);
		c->onfalse=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)c;
	}
#line 1821 "parser.tab.c"
    break;

  case 54:
#line 375 "parser.y"
                        {
		(yyval.expression)=(yyvsp[0].expression);
	}
#line 1829 "parser.tab.c"
    break;

  case 55:
#line 378 "parser.y"
                                                              {
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[-3].expression);
		c->ontrue=(yyvsp[-1].expression);
		c->onfalse=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)c;
	}
#line 1842 "parser.tab.c"
    break;

  case 56:
#line 386 "parser.y"
                                             {
		Conditional* c=new_conditional();
		ADD_DEBUG_INFO(c)
		c->condition=(yyvsp[-2].expression);
		c->ontrue=(yyvsp[0].expression);
		c->onfalse=(Expression*)new_empty();
		(yyval.expression)=(Expression*)c;
	}
#line 1855 "parser.tab.c"
    break;

  case 57:
#line 396 "parser.y"
                               {
		pointers_vector_push((yyvsp[-2].arguments), (yyvsp[0].expression));
		(yyval.arguments)=(yyvsp[-2].arguments);
	}
#line 1864 "parser.tab.c"
    break;

  case 58:
#line 400 "parser.y"
                   {
		vector* arguments=malloc(sizeof(vector));
		CHECK_ALLOCATION(arguments);
		vector_init(arguments, sizeof(Argument), 8);
		pointers_vector_push(arguments, (yyvsp[0].expression));
		(yyval.arguments)=arguments;
	}
#line 1876 "parser.tab.c"
    break;

  case 59:
#line 409 "parser.y"
             {
		Argument* a=new_argument();
		Name* name=(Name*)(yyvsp[0].expression);
		name_to_argument(name, a);
		delete_expression((Expression*)name);
		(yyval.expression)=(Expression*)a;
	}
#line 1888 "parser.tab.c"
    break;

  case 60:
#line 416 "parser.y"
                            { (yyval.expression)=(yyvsp[0].expression); }
#line 1894 "parser.tab.c"
    break;

  case 61:
#line 417 "parser.y"
                            { (yyval.expression)=(yyvsp[0].expression); }
#line 1900 "parser.tab.c"
    break;

  case 62:
#line 420 "parser.y"
                      {
		VariadicArgument* a=new_variadic_argument();
		Name* name=(Name*)(yyvsp[-1].expression);
		name_to_argument(name, (Argument*)a);
		delete_expression((Expression*)name);
		(yyval.expression)=(Expression*)a;
	}
#line 1912 "parser.tab.c"
    break;

  case 63:
#line 429 "parser.y"
                            {
		OptionalArgument* a=new_optional_argument();
		Name* name=(Name*)(yyvsp[-2].expression);
		name_to_argument(name, (Argument*)a);
		delete_expression((Expression*)name);
		a->value=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)a;
	}
#line 1925 "parser.tab.c"
    break;

  case 64:
#line 439 "parser.y"
                                           {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		f->arguments=*(vector*)(yyvsp[-3].arguments);
		free((yyvsp[-3].arguments));

		f->variadic=false;
		f->body=(yyvsp[0].expression);
		if(process_function_declaration_arguments(f)){
			// function's arguments are formed incorrectly
			delete_expression((Expression*)f);
			YYABORT;
		}
		// one argument function's don't need to be processed using this function
		(yyval.expression)=(Expression*)f;
	}
#line 1946 "parser.tab.c"
    break;

  case 65:
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
		ONE_ARGUMENT_FUNCTION((yyval.expression), (yyvsp[-2].expression), (yyvsp[0].expression), false)
	}
#line 1972 "parser.tab.c"
    break;

  case 66:
#line 476 "parser.y"
                                         {
		ONE_ARGUMENT_FUNCTION((yyval.expression), (yyvsp[-3].expression), (yyvsp[0].expression), true)
		#undef ONE_ARGUMENT_FUNCTION
	}
#line 1981 "parser.tab.c"
    break;

  case 67:
#line 480 "parser.y"
                           {
		FunctionDeclaration* f=new_function_declaration();
		ADD_DEBUG_INFO(f)
		f->variadic=false;
		f->body=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)f;
	}
#line 1993 "parser.tab.c"
    break;

  case 68:
#line 489 "parser.y"
            {
		IntLiteral* l=new_int_literal();
		ADD_DEBUG_INFO(l)
		l->value=(yyvsp[0].ival);
		(yyval.expression)=(Expression*)l;
	}
#line 2004 "parser.tab.c"
    break;

  case 69:
#line 495 "parser.y"
                { 
		FloatLiteral* l=new_float_literal();
		ADD_DEBUG_INFO(l)
		l->value=(yyvsp[0].fval);
		(yyval.expression)=(Expression*)l;
	}
#line 2015 "parser.tab.c"
    break;

  case 70:
#line 501 "parser.y"
                 { 
		StringLiteral* l=new_string_literal();
		ADD_DEBUG_INFO(l)
		l->value=(yyvsp[0].sval);
		(yyval.expression)=(Expression*)l;
	}
#line 2026 "parser.tab.c"
    break;

  case 71:
#line 509 "parser.y"
                     {
		NullLiteral* n=new_null_literal();
		ADD_DEBUG_INFO(n)
		(yyval.expression)=(Expression*)n;
	}
#line 2036 "parser.tab.c"
    break;

  case 72:
#line 516 "parser.y"
             {
		Name* n=new_name();
		ADD_DEBUG_INFO(n)
		n->value=(yyvsp[0].sval);
		(yyval.expression)=(Expression*)n;
	  }
#line 2047 "parser.tab.c"
    break;

  case 73:
#line 525 "parser.y"
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
#line 2065 "parser.tab.c"
    break;

  case 74:
#line 539 "parser.y"
        {
		Assignment* a=new_assignment();
		ADD_DEBUG_INFO(a)
		a->left=(Expression*)(yyvsp[-2].expression);
		a->right=(yyvsp[0].expression);
		a->used_in_closure=false;
		(yyval.expression)=(Expression*)a;
	}
#line 2078 "parser.tab.c"
    break;

  case 75:
#line 550 "parser.y"
        {
		MacroDeclaration* md=new_macro_declaration();
		ADD_DEBUG_INFO(md)
		md->left=(Macro*)(yyvsp[-2].expression);
		md->right=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)md;
	}
#line 2090 "parser.tab.c"
    break;

  case 76:
#line 559 "parser.y"
                                 {
		FunctionCall* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->called=(yyvsp[-3].expression);
		c->arguments=(Block*)(yyvsp[-1].expression);
		c->arguments->type=e_table_literal;
		(yyval.expression)=(Expression*)c;
	}
#line 2103 "parser.tab.c"
    break;

  case 77:
#line 567 "parser.y"
                             {
		FunctionCall* c=new_function_call();
		ADD_DEBUG_INFO(c)
		c->called=(yyvsp[-2].expression);
		c->arguments=(Block*)new_block();
		(yyval.expression)=(Expression*)c;
	}
#line 2115 "parser.tab.c"
    break;

  case 78:
#line 576 "parser.y"
                                                {
		Message* m=new_message();
		ADD_DEBUG_INFO(m)
		m->messaged_object=(yyvsp[-5].expression);
		m->message_name=(Name*)(yyvsp[-3].expression);
		m->arguments=(Block*)(yyvsp[-1].expression);
		m->arguments->type=e_table_literal;
		(yyval.expression)=(Expression*)m;
	}
#line 2129 "parser.tab.c"
    break;

  case 79:
#line 585 "parser.y"
                                            {
		Message* m=new_message();
		ADD_DEBUG_INFO(m)
		m->messaged_object=(yyvsp[-4].expression);
		m->message_name=(Name*)(yyvsp[-2].expression);
		m->arguments=(Block*)new_block();
		(yyval.expression)=(Expression*)m;
	}
#line 2142 "parser.tab.c"
    break;

  case 80:
#line 596 "parser.y"
        {
		Binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=(yyvsp[-2].expression);
		u->op=(yyvsp[-1].sval);
		u->right=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)u;
	}
#line 2155 "parser.tab.c"
    break;

  case 81:
#line 605 "parser.y"
        {
		Binary* u=new_binary();
		ADD_DEBUG_INFO(u)
		u->left=(yyvsp[-2].expression);
		u->op=strdup("-");
		u->right=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)u;
	}
#line 2168 "parser.tab.c"
    break;

  case 82:
#line 616 "parser.y"
        {
		Prefix* p=new_prefix();
		ADD_DEBUG_INFO(p)
		p->op=strdup("!");
		p->right=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)p;
	}
#line 2180 "parser.tab.c"
    break;

  case 83:
#line 624 "parser.y"
        {
		Prefix* p=new_prefix();
		ADD_DEBUG_INFO(p)
		p->op=strdup("-");
		p->right=(yyvsp[0].expression);
		(yyval.expression)=(Expression*)p;
	}
#line 2192 "parser.tab.c"
    break;

  case 84:
#line 634 "parser.y"
        {
		ReturnIfError* r=new_return_if_error();
		ADD_DEBUG_INFO(r)
		r->value=(yyvsp[-2].expression);
		(yyval.expression)=(Expression*)r;
	}
#line 2203 "parser.tab.c"
    break;


#line 2207 "parser.tab.c"

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
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

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
            yymsg = YY_CAST (char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

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


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
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
                  yystos[+*yyssp], yyvsp);
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
		// TODO: some better syntax and file reading errors reporting system
		// that would allow ignoring this error for prelude and also generating error
		// object for the user
		// printf("I can't open the file!\n");
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
