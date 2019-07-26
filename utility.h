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

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

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
char* string_add(const char* a, const char* b);
char* string_repeat(const char* str, int times);

#endif