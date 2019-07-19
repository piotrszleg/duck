#ifndef UTILITY_H
#define UTILITY_H

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "error/error.h"

// creates string variable str, executes body and frees the string afterwards
#define USING_STRING(string_expression, body) { char* str=string_expression; body; free(str); }

#define CHECK_ALLOCATION(value) \
    if(value==NULL) { \
        THROW_ERROR(MEMORY_ALLOCATION_FAILURE, "Memory allocation failure in function %s", __FUNCTION__); \
    }

#define REQUIRE(predicate, cause) if(!(predicate)) { RETURN_ERROR("WRONG_ARGUMENT", cause, "Requirement of function %s wasn't satisified: %s", __FUNCTION__, #predicate); }
#define REQUIRE_TYPE(o, t) if(o.type!=t) { \
    RETURN_ERROR("WRONG_OBJECT_TYPE", o, "Wrong type of \"%s\" in function %s, it should be %s.", #o, __FUNCTION__, OBJECT_TYPE_NAMES[t]); }

#define REQUIRE_ARGUMENT_TYPE(o, t) if(o.type!=t) { \
    RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Wrong type of argument \"%s\" passed to function %s, it should be %s.", #o, __FUNCTION__, OBJECT_TYPE_NAMES[t]); }

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

#define EQUALS_STRING(object, str) (object.type==t_string && strcmp(object.text, str)==0)

#define DEBUG_PRINT printf("<%i>\n", __COUNTER__);

char* fgets_no_newline(char *buffer, size_t buflen, FILE* fp);
int nearest_power_of_two(int number);
int sign(int x);

#define CONSTANT_REPLACEMENT_PAIR(to_replace, replacement) \
    {to_replace, sizeof(to_replace)-1, replacement, sizeof(replacement)-1}

typedef struct {
    char* to_replace;
    unsigned to_replace_length;
    char* replacement;
    unsigned replacement_length;
} ReplacementPair;

typedef struct {
    char* position;
    int pair_index;
} Occurrence;

char* string_replace_multiple(char* original, ReplacementPair* replacement_pairs, int replacement_pairs_count);
char* string_add(char* a, char* b);
char* string_repeat(char* str, int times);

#endif