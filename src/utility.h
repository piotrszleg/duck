#ifndef UTILITY_H
#define UTILITY_H

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "error/error.h"

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field_name) (long long)&((type*)0)->field_name
#endif

// creates string variable str, executes body and frees the string afterwards
#define USING_STRING(string_expression, body) { char* str=string_expression; body; free(str); }

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

#define DEBUG_PRINT printf("<%i>\n", __COUNTER__);

char* fgets_no_newline(char *buffer, size_t buflen, FILE* fp);
char* read_entire_file(FILE* fp);
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
bool strings_counted_equal(char* a, char* b, size_t count);
void* copy_memory(void* source, size_t size);

#endif