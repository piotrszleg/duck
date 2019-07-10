#include "utility.h"

// source: https://stackoverflow.com/questions/1694036/why-is-the-gets-function-so-dangerous-that-it-should-not-be-used
char* fgets_no_newline(char *buffer, size_t buflen, FILE* fp) {
    if (fgets(buffer, buflen, fp) != 0)
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        return buffer;
    } else{ 
        return NULL;
    }
}

int nearest_power_of_two(int number){
    int i;
    for(i=1; i<=number;i*=2);
    return i;
}

static bool strings_counted_equal(char* a, char* b, int count){
    for(int i=0; i<count; i++){
        if(a[i]!=b[i]){
            return false;
        }
    }
    return true;
}

int sign(int x){
    return (x > 0) - (x < 0);
}

char* string_replace_multiple(char* original, ReplacementPair* replacement_pairs, int replacement_pairs_count){
    int original_length=strlen(original);
    int new_size=original_length;
    int occurrences_size=16;
    Occurrence* occurrences=malloc(sizeof(Occurrence)*occurrences_size);
    CHECK_ALLOCATION(occurrences)
    int occurrences_count=0;

    for(int i=0; i<original_length; i++){
        for(int p=0; p<replacement_pairs_count; p++){
            if(strings_counted_equal(&original[i], replacement_pairs[p].to_replace, replacement_pairs[p].to_replace_length)){
                if(occurrences_count>=occurrences_size){
                    occurrences_size*=2;
                    occurrences=realloc(occurrences, sizeof(Occurrence)*occurrences_size);
                    CHECK_ALLOCATION(occurrences)
                }
                new_size+=replacement_pairs[p].replacement_length-replacement_pairs[p].to_replace_length;
                Occurrence occurrence={&original[i], p};
                occurrences[occurrences_count]=occurrence;
                occurrences_count++;
                i+=replacement_pairs[p].to_replace_length-1;
            }
        }
    }
    new_size++;// for null terminator
    char* result=malloc(new_size);
    char* result_write=result;
    char* original_read=original;
    
    for(int o=0; o<occurrences_count; o++){
        strncpy(result_write, original_read, occurrences[o].position-original_read);
        result_write+=occurrences[o].position-original_read;
        original_read=occurrences[o].position+replacement_pairs[occurrences[o].pair_index].to_replace_length;
        strcpy(result_write, replacement_pairs[occurrences[o].pair_index].replacement);
        result_write+=replacement_pairs[occurrences[o].pair_index].replacement_length;
    }
    strncpy(result_write, original_read, original_length-(original_read-original));
    result[new_size-1]='\0';
    free(occurrences);
    return result;
}